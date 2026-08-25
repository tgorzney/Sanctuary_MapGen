# STEP153 — Every file/folder picker becomes a true single button

**Layer:** UI. **Domain:** `src/ui/FilePathPicker_UI.h`/`.cpp` (the shared control), plus its 12
real call sites (see below). **Sequence:** independent of STEP150-152 (different files). Note:
`src/ui/MarkersTab_Globals_UI.cpp` is one of the 12 call sites and sits inside a concurrent peer
session's claimed Marker-tab territory — ping them (`map-generator-ea`) before touching that one
file specifically; every other file in this ticket is clear.

## Root problem
The human's explicit, repeated instruction: *"ANYTHING that selects a file/s or Folder/s should be
the single button widget."* `DrawFilePathPicker` (`FilePathPicker_UI.h:74`, `.cpp:30-57`) is already
the app's one shared/universal path-editing control (its own header comment says every path a tab
exposes is meant to go through it) — but it renders FOUR always-visible elements on one row, not a
single button:
```cpp
ImGui::TextUnformatted(label);                     // 1. always-visible label text
if (ImGui::Button(buttonLabel)) ...                 // 2. "Browse..." button
if (options.bClearButtonShown) { Button("x", ...) } // 3. separate clear button
ImGui::TextUnformatted(shortLabel.c_str());         // 4. separate shortened-path text + tooltip
```
This is the pattern behind the Layer Editor's "Import RAW" row the human flagged directly, and 11
other call sites across the app render the identical four-element row. Elsewhere in the codebase
(`FilesTab_Browse_UI.cpp:88-94`, `DrawFilesTabOpenButton`) there's a genuinely single-button pattern
— one `ImGui::Button`, nothing else drawn beside it, opens the native dialog directly on click — but
it's a narrow, Files-tab-internal helper (hardcoded to one file kind, static label, no current-path
display) used exactly once. Nothing in the codebase today is both (a) truly one button and (b)
general enough to be the universal control.

## Fix
Redesign `DrawFilePathPicker` itself into a true single button, and let all 12 real call sites
inherit the fix by construction — do not patch the Layer Editor's Import RAW row in isolation, the
human's own wording ("ANYTHING") is explicit that this is a systemic fix, not a one-off.

1. **Rendering**: exactly one `ImGui::Button` per picker, nothing else drawn alongside it at rest.
   The button's own label is the picker's existing `label` parameter (e.g. "Import RAW", "Gamedata
   Folder") — do not try to cram a long/variable path into the button's visible text. The CURRENT
   path (or "None" when empty) is shown via `ImGui::SetTooltip` on hover, using the existing
   shortened/`maximumLabelCharacterCount`-trimmed path logic already in the file — reuse it, don't
   rewrite it.
2. **Click behavior**: unchanged contract — clicking reports `bBrowseRequested` back to the caller
   exactly as today (the widget itself still does not open a platform dialog directly, per its own
   documented design intent, `FilePathPicker_UI.h:6-10` — the host runs `Io::FileDialog`/
   `RunBrowseDialog`). Only the RENDERING collapses to one button; the request/response contract
   (`FilePathPickerResult`, `options.RequestFilePath`, `bBrowseRequested`) stays as-is so callers
   don't need logic changes, only to stop expecting multiple drawn elements.
3. **Clearing an existing path**: currently a separate always-visible "x" button
   (`options.bClearButtonShown`). Move this to a right-click context menu on the SAME button (e.g.
   `ImGui::BeginPopupContextItem()` with a "Clear" entry) when `bClearButtonShown` is true — this
   keeps exactly one widget drawn at rest while preserving the ability to clear. Do not drop clear
   functionality outright for any caller that currently sets `bClearButtonShown = true`; check each
   of the 12 call sites for whether they rely on it before finalizing.
4. **Folder vs. file mode**: verify (don't assume) whether `FilePathPickerOptions`/the caller-supplied
   `RequestFilePath` seam already distinguishes file vs. folder selection per call site (several of
   the 12 — "Gamedata Folder", "Game Install Root", "Destination Map Folder", "Environment Pack" —
   are directory pickers, not file pickers). The single-button redesign must preserve whatever that
   distinction already is; this ticket does not change what gets selected, only how the control is
   drawn.
5. **Migrate every call site** — do not leave any of the 12 on the old rendering:
   - Direct `DrawFilePathPicker` calls: `ArmiesTab_UI.cpp:69`, `LayerEditor_Group_UI.cpp:54`
     (Import RAW — the one the human named directly), `MarkersTab_Globals_UI.cpp:14` (coordinate
     with the peer session first, see header), `ScenariosTab_RuntimeScript_UI.cpp:47`,
     `StratumsTab_Material_UI.cpp:31`, `StratumsTab_UI.cpp:20`.
   - Indirect via the `DrawFilesTabPathRow` wrapper (`FilesTab_Browse_UI.cpp:75`, itself forwarding
     into `DrawFilePathPicker` — fixing the shared control fixes all of these for free, verify
     nothing in the wrapper ALSO draws a redundant extra element of its own that would survive the
     control's redesign): `FilesTab_Draw_UI.cpp:57`, `FilesTab_Draw_UI.cpp:69`,
     `FilesTab_ScenarioExportRow_Draw_UI.cpp:30`, `:71`, `:74`.
   - Do NOT touch `DrawFilesTabOpenButton` (`FilesTab_Browse_UI.cpp:88-94`) or its one call site
     (`FilesTab_Draw_UI.cpp:45`) — it is already a correct single button, just narrower in scope;
     leave it alone rather than merging the two, unless doing so is trivial and strictly additive
     (your call once you see the real code, but don't force a merge that isn't clean).

## Explicit out-of-scope
- Any change to WHAT gets selected (extensions, file vs. folder, validation) at any of the 12 call
  sites — this ticket is rendering-only.
- Generalizing `DrawFilesTabOpenButton` or merging it with `DrawFilePathPicker` unless trivial.
- Any non-file/folder button elsewhere in the UI that merely happens to be labeled like a picker
  (e.g. `StratumsTab_Material_UI.cpp:40`'s `ImportedMaskModeLabel` mode-toggle button — not a file
  picker, do not touch).

## Acceptance test
Extend or add a `FilePathPicker_UI_Test.cpp` (find the real existing test file for this control
first): exactly one interactive widget is drawn per picker at rest (assert via imgui's item count/
ID stack the way other headless widget tests in this codebase already do — match their pattern,
don't invent a new assertion style); the tooltip shows the current path (and "None"/empty-state
text) on hover; a right-click on the button surfaces a working Clear when `bClearButtonShown` is
true and does not appear when false; `bBrowseRequested`/`FilePathPickerResult` contract is
unchanged from before this ticket (existing tests for callers like Import RAW should need at most a
rendering-assertion update, not a logic rewrite — if any caller's test needs real logic changes,
stop and reconsider whether this ticket accidentally changed behavior beyond rendering).

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green.

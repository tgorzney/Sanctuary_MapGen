# STEP43 — Compact Open Sanmap button (single click, no Browse step)

**Layer:** UI. **Domain:** Files tab. **Consulted:** SanGen UI Expert (read-only, this session).

## Problem
`DrawOpenSection` (`src/ui/FilesTab_Draw_UI.cpp:83-97`) currently needs two clicks to open a
file: a path row with a "Browse..." button (`DrawFilesTabPathRow`, wraps
`FilePathPicker_UI.h`) sets `state.sanmapPath`, then a separate "Open Sanmap File" button
actually loads it. Human instruction: collapse this into one click — click "Open Sanmap File",
the native dialog opens directly, and on selection the file loads immediately. No Browse
button, no editable path text field for this row.

Scope is the Sanmap Open row only. The SupCom Lua row and the Export Folder row keep their
existing `DrawFilesTabPathRow` picker pattern unchanged — this is not a widget-library change.

## Ruling (UI Expert consult)
1. **No new "currently loaded" label.** `DrawLogSection` already shows `RunOpenSanmap`'s log,
   which names the loaded file. Don't add a second source of truth for the same fact.
2. **`RunFilesTabAction`/`RunOpenSanmap` signatures do not change.** They keep reading
   `state.sanmapPath` exactly as today (`FilesTab_UI.h:130-131`,
   `FilesTab_Actions_UI.cpp:30-46`) — required so `FilesTab_UI_Test.cpp`'s existing
   hand-set-state-then-drive-the-action pattern keeps working untouched. The new button handler
   sets `state.sanmapPath` from the dialog result, then calls the existing action.
3. **New function lives in `FilesTab_Browse_UI.h`/`.cpp`**, the one place this tab already
   touches `Io::FileDialog` — not inlined into `FilesTab_Draw_UI.cpp` (that file's own header
   comment: "the draw path decides nothing"). Add:
   ```cpp
   // Draws the button itself and, on click, runs the native Open dialog directly — no separate
   // Browse step, no picker row. True when the user picked a file (`filePath` was updated).
   bool DrawFilesTabOpenButton(const char* label, std::string& filePath);
   ```
   Implement by reusing the existing private `RunBrowseDialog(FilesTabBrowseKind::SanmapDocument,
   filePath, chosenPath)` (`FilesTab_Browse_UI.cpp:40-46`) — `BuildDialogRequest`'s
   `SanmapDocument` branch is already correct, no new filter/title logic needed. Deliberately
   skip the extension fence (`BuildPickerOptions`/`ApplyChosenFilePath`'s allowed-extensions
   check) — the existing row comment already establishes `.sanmap` is intentionally unfenced
   (the importer resolves either a document or its folder).
4. **`DrawOpenSection` replacement** (`FilesTab_Draw_UI.cpp:83-97`): keep the checkbox and the
   `fields == nullptr` warning exactly as they are, before the button. Replace the
   `DrawFilesTabPathRow` + `DrawActionButton(OpenSanmap, ...)` pair with:
   ```cpp
   if (DrawFilesTabOpenButton(FilesTabActionLabel(FilesTabAction::OpenSanmap), state.sanmapPath)) {
       const bool bSucceeded = RunFilesTabAction(FilesTabAction::OpenSanmap, state, recipe, fields);
       if (bSucceeded && previewDriver != nullptr) previewDriver->RequestMapUpdate();
   }
   ```
5. **⚠️ Do not drop `RequestMapUpdate()`.** `DrawActionButton` (`FilesTab_Draw_UI.cpp:26-33`) is
   shared between `OpenSanmap` and `ImportSupComLua` today; once Open stops going through it,
   that shared preview-invalidation call must be reproduced inline at the new call site (as
   shown in the snippet above) or the map preview goes stale after an Open with no compiler
   error to catch the omission. `ImportSupComLua` keeps using `DrawActionButton` unchanged.

## Files touched
- `src/ui/FilesTab_Browse_UI.h` — declare `DrawFilesTabOpenButton`
- `src/ui/FilesTab_Browse_UI.cpp` — implement it, reusing `RunBrowseDialog`
- `src/ui/FilesTab_Draw_UI.cpp` — `DrawOpenSection` rewritten per §4 above
- No changes to `FilesTab_Actions_UI.cpp`, `FilesTab_UI.h`, or any test file's expectations of
  `RunFilesTabAction`'s signature.

## Verify
Full solo rebuild + `ctest -C Debug`. `FilesTab_UI_Test.cpp` must stay green unmodified (it
drives `RunFilesTabAction` directly, never the new button). This is a UI-composition change
only — no automated test can click the real native dialog, so manual confirmation that the
button opens the dialog and loads on selection is the human's own job, not the coder's or
verifier's (see memory: no interactive testing by AI).

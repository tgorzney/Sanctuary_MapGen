# STEP258 — Marker Link header: purple color match + double-click-to-toggle / single-click-to-select

**Layer:** UI. **Domain:** `src/ui/Section_UI.h/.cpp` (shared opt-in interaction field, every other
call site unaffected — verified against all 50+ live `DrawSectionBegin` call sites),
`src/ui/MarkersTab_Links_UI.h/.cpp` (the one opted-in call site + the new select-all pure function),
`src/ui/Section_UI_Test.cpp`, `src/ui/MarkersTab_Links_UI_Test.cpp` (new coverage). **Executor:**
SanGen Coder.

## Part 1 — Color

`kMarkerLinkSectionHeaderColor` (`MarkersTab_Links_UI.h:69`, `0xFFB366CCu`) becomes a purple that
shares the exact saturation/value of the stock blue used by every ordinary Group/Section header in
the app.

Verified independently (not just trusting the human's math): no custom style table exists anywhere
in this codebase (only `ImGui::StyleColorsDark()` is called, `Application_Window_UI.cpp:48`), and
`build/_deps/imgui-src/imgui_draw.cpp:217` (`StyleColorsDark`) sets
`colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f)` — i.e. RGB(66,150,250) at 0-255 scale.
`Ui::ResolveWidgetColor` (`RtToggleWidget_UI.cpp:12-16`) returns `style.trackColor` **verbatim**
whenever it is not `kThemeColor` (0), so `kMarkerLinkSectionHeaderColor` is already, today, drawn as
a fixed literal regardless of hover state — the hover-shading branch in `Section_UI.cpp:56-58` is a
dead argument for this call site both before and after this change. This confirms the reference
color to match is the flat, alpha-31 dark-theme `ImGuiCol_Header` RGB triple, independent of any
hover/active variant.

HSV of RGB(66,150,250)/255 = (0.2588, 0.5882, 0.9804): max=B=0.9804, min=R=0.2588, delta=0.7216.
V = 0.9804. S = delta/max = 0.7361. H = 60*(4 + (r-g)/delta) = 60*(4 - 0.4566) = 212.6°. Matches the
human's figures exactly.

Rotated to 270° (standard violet — reads unambiguously purple against this app's dark theme, and is
maximally far from the existing blue hue so there is no risk of the two being confused at a glance)
at the identical S=0.7361, V=0.9804: C = V*S = 0.7216, X = C*(1-|((270/60) mod 2)-1|) = C*0.5 = 0.3608,
m = V-C = 0.2588. H in [240°,300°) → (R',G',B') = (X,0,C) = (0.3608, 0, 0.7216).
R = (0.3608+0.2588)*255 = 158.0 → 0x9E. G = (0+0.2588)*255 = 66.0 → 0x42. B = (0.7216+0.2588)*255 =
250.0 → 0xFA. RGB = 0x9E42FA. `PackedColor` is `0xAABBGGRR` (confirmed against the existing constant's
own decode: `0xFFB366CCu` → A=FF,B=B3,G=66,R=CC → RGB(204,102,179), a plausible "muted violet/rose" —
format check passes) and against `WidgetHelpers_UI.h:20-24`'s own "byte-identical to IM_COL32" note.
Alpha stays `0xFF` (the pre-existing constant's own alpha, unaffected by this rotation). Final:

```cpp
// A named PackedColor distinct from every colorAlloy/Plasma/Spawn default and from kThemeColor's own
// resolved value (Constitution §8 — a UI-chrome tweakable, not a PARAMS/recipe value, same tier as
// kMarkerLayerHeaderExtraCombinedWidthPixels) — 0xAABBGGRR. STEP258: the SAME hue-family target as
// ordinary Group/Section headers elsewhere in this tab. DrawSectionBegin's own default trackColor ==
// kThemeColor resolves, via ResolveWidgetColor, to Dear ImGui's stock dark-theme ImGuiCol_Header ==
// RGB(0.26,0.59,0.98) == RGB(66,150,250), HSV ~= (212.6 deg, 0.736, 0.980) (imgui_draw.cpp's own
// StyleColorsDark — this repo never overrides it, Application_Window_UI.cpp:48 is the only style
// call). Rotated to a purple hue (270 deg, standard violet — maximally distinct from the existing
// blue) at the IDENTICAL saturation/value: RGB(158,66,250) = 0x9E42FA. Alpha kept at the pre-existing
// 0xFF — this constant is never kThemeColor, so ResolveWidgetColor (RtToggleWidget_UI.cpp) always
// returns it verbatim regardless of hover state; Section_UI.cpp's own hover/HeaderHovered branch is a
// dead argument for any non-kThemeColor trackColor, unchanged by this ticket. Was 0xFFB366CCu ("a
// muted violet/rose") before STEP258 — replaced outright, per direct human instruction to match the
// stock headers' own S/V with a purple hue instead of blue, not layered alongside the old value.
inline constexpr PackedColor kMarkerLinkSectionHeaderColor = 0xFFFA429Eu;
```

(`MarkersTab_Links_UI.h`, replaces line 69's constant and widens its comment block, lines 65-69.)

## Part 2 — Interaction model: double-click toggles, single-click selects (Link header only)

### Design decision 1 — the shared opt-in shape (`Section_UI.h/.cpp`)

Verified before touching shared code: `grep -rn "DrawSectionBegin(" src/` finds **every** call site
in the app uses the plain `bool DrawSectionBegin(...)` return (`if (DrawSectionBegin(...))` /
`if (!DrawSectionBegin(...)) return`) — none take the address of the function, none rely on its exact
parameter types beyond call compatibility. `StepSectionHeader` is called directly only from
`Section_UI.cpp` (production) and `Section_UI_Test.cpp` (three existing calls, all passing a bare
`bool` literal). Both facts constrain the change to be pure signature-widening with default arguments
— zero edits required at any of the 50+ existing call sites.

`SectionOptions` gains one opt-in field (default `false`, so every existing header is byte-identical
in behavior):

```cpp
// STEP258 — opt-in only: default false preserves every existing header's single-click-to-toggle
// behavior UNCHANGED. When true, ONLY the second click of a double-click (the frame Dear ImGui
// itself resolves as click-count 2 — its own io.MouseClickedCount, imgui.cpp's UpdateMouseInputs)
// toggles bOpen; a plain single click (count 1) does not touch bOpen at all and instead reports
// bPlainSingleClicked back through SectionChange/DrawSectionBegin's own out-param, for a caller that
// wants a single click to mean something OTHER than expand/collapse (STEP258's own Link-header
// "select all its instances" use, MarkersTab_Links_UI.h). A triple-plus click (count >= 3) does
// neither — inert by construction, not a defect: no repeated select-all, no accidental extra toggle.
// No other header in the app sets this — LinkSectionHeaderOptions() is, as of STEP258, the ONLY
// SectionOptions call site in the entire codebase that does.
bool bDoubleClickToggle = false;
```

`SectionChange` gains one opt-in field:

```cpp
// STEP258 — true on the exact frame a header configured with SectionOptions::bDoubleClickToggle
// received a PLAIN single click (click-count 1, not the second click of a double-click) — the
// caller's cue to run ITS OWN single-click action instead of a toggle. Always false whenever
// bDoubleClickToggle is false (every pre-existing header): those toggle on any click and never
// populate this field, so an existing caller that ignores it loses nothing.
bool bPlainSingleClicked = false;
```

`StepSectionHeader` widens from a raw `bool bHeaderClicked` to an `int headerClickCount` (0/1/2+,
Section_UI.cpp is the only place that ever derives this from real mouse state) plus the new opt-in
flag. `true`/`false` bool literals implicitly convert to `1`/`0`, so all three existing
`Section_UI_Test.cpp` call sites keep compiling with byte-identical behavior (verified: with
`bDoubleClickToggle` defaulted false, `bTogglingClick = (headerClickCount >= 1)` reduces to exactly
the old `if (bHeaderClicked)`):

```cpp
// One frame of header interaction: pure so a tab's open/closed behavior is testable without an
// imgui frame. `headerClickCount` is 0 (no click), 1 (a plain single click) or 2+ (the second-or-
// later click of a multi-click) this frame — Section_UI.cpp is the only caller that ever derives
// this from real mouse state (ImGui::GetMouseClickedCount). `bDoubleClickToggle` mirrors
// SectionOptions of the same name (STEP258): false (every pre-existing header) toggles on ANY
// click, exactly today's behavior; true toggles ONLY on count==2 and reports a count==1 click via
// bPlainSingleClicked instead of touching bOpen (count >= 3 does neither, see SectionOptions' own
// comment).
inline SectionChange StepSectionHeader(SectionState& state, int headerClickCount,
                                       bool bDoubleClickToggle = false) {
    SectionChange change;
    const bool bTogglingClick = bDoubleClickToggle ? (headerClickCount == 2) : (headerClickCount >= 1);
    if (bTogglingClick) {
        state.bOpen = !state.bOpen;
        change.bOpenChanged = true;
    }
    change.bPlainSingleClicked = bDoubleClickToggle && headerClickCount == 1;
    change.bBodyVisible = state.bOpen;
    return change;
}
```

`DrawSectionBegin` widens with one trailing default-`nullptr` out-param (every existing call site
unaffected):

```cpp
// Draws the header bar and returns true when the BODY should be drawn. Call DrawSectionEnd only
// on the frames this returned true (the imgui Begin/End convention):
//
//   if (Ui::DrawSectionBegin("Sun", tabState.sunSection)) { ...controls...; Ui::DrawSectionEnd(); }
//
// `outPlainSingleClicked`, if non-null, is written every call (STEP258): true only on the frame
// SectionChange::bPlainSingleClicked is true (see there). Every EXISTING call site passes nullptr
// (the default) and is completely unaffected; only a caller that also set
// SectionOptions::bDoubleClickToggle has any reason to read it.
bool DrawSectionBegin(const char* label, SectionState& state,
                      const SectionOptions& options = SectionOptions(),
                      const WidgetStyle& style = WidgetStyle(),
                      bool* outPlainSingleClicked = nullptr);
```

`Section_UI.cpp` — the only new imgui-touching lines (the rest of the function body is unchanged):

```cpp
bool DrawSectionBegin(const char* label, SectionState& state, const SectionOptions& options,
                      const WidgetStyle& style, bool* outPlainSingleClicked) {
    if (options.topSpacing > 0.0f) ImGui::Dummy(ImVec2(0.0f, options.topSpacing));
    ImGui::PushID(label);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float rawBarWidth = ImGui::GetContentRegionAvail().x - options.reservedRightWidth;
    const float barWidth  = rawBarWidth > 1.0f ? rawBarWidth : 1.0f;
    const float barHeight = ResolveWidgetTrackHeight(style);
    ImGui::InvisibleButton("##header", ImVec2(barWidth, barHeight));
    const bool bHeaderClicked = ImGui::IsItemClicked();
    // STEP258 — GetMouseClickedCount is only read when bHeaderClicked is true this SAME frame: it is
    // computed fresh on every real mouse-press transition (imgui.cpp's UpdateMouseInputs sets
    // io.MouseClickedCount[i] in the same block as io.MouseClicked[i]), so this can never read a
    // stale count left over from an earlier, unrelated click streak.
    const int headerClickCount = bHeaderClicked ? ImGui::GetMouseClickedCount(ImGuiMouseButton_Left) : 0;
    const bool bHeaderHovered = ImGui::IsItemHovered();

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(origin, ImVec2(origin.x + barWidth, origin.y + barHeight),
                            ResolveWidgetColor(style.trackColor,
                                               bHeaderHovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header),
                            ResolveHeaderRounding(options, style));

    const SectionChange change = StepSectionHeader(state, headerClickCount, options.bDoubleClickToggle);
    if (outPlainSingleClicked) *outPlainSingleClicked = change.bPlainSingleClicked;
    if (options.bArrowShown) DrawDisclosureArrow(drawList, origin, barHeight, state.bOpen);
    const float labelLeftX = origin.x + (options.bArrowShown ? barHeight : ImGui::GetStyle().FramePadding.x);
    drawList->AddText(ImVec2(labelLeftX, origin.y + (barHeight - ImGui::GetTextLineHeight()) * 0.5f),
                      ResolveWidgetColor(kThemeColor, ImGuiCol_Text), label);

    ImGui::PopID();
    if (change.bBodyVisible) IndentSectionBody(options);
    return change.bBodyVisible;
}
```

**Why click 1 selects and click 2 (the double-click) only toggles, never re-selects — the human's own
open question.** A genuine double-click gesture produces two real mouse-down events. On the first
(`headerClickCount==1`), `bPlainSingleClicked` is true — select-all fires. On the second
(`headerClickCount==2`, arriving within imgui's own `io.MouseDoubleClickTime`/`MaxDist`), the toggle
fires instead and `bPlainSingleClicked` is false, so select-all does **not** fire a second time. This
needs no extra debounce/timer: it falls directly out of imgui's own click-count semantics. Rationale:
this matches the universal "file-manager" convention (a single click always selects, including the
first click of what turns out to be a double-click) rather than a jarring double-fire or a delayed-
selection UX; and it is the only choice that requires zero new state (a "wait N ms to see if this
becomes a double-click" delayed-single-click design would need a timer field this pure/testable
widget library has no precedent for, e.g. `imgui.cpp:10786`'s own
`GetItemClickedCountWithSingleClickDelay` helper — deliberately NOT used here, out of scope, adds a
delay a single-target-per-click selection UI doesn't need).

### Design decision 2 — `LinkSectionHeaderOptions()` opts in

```cpp
inline SectionOptions LinkSectionHeaderOptions() {
    SectionOptions options;
    options.reservedRightWidth = kMarkerLinkHeaderClusterWidthPixels;
    // STEP258 — opt-in double-click-to-toggle: a plain single click on a Link header selects every
    // Manual Instance tagged to it instead of collapsing/expanding it (DrawMarkerLinksSection's own
    // bPlainSingleClicked handling, MarkersTab_Links_UI.cpp). The ONLY SectionOptions call site in
    // the app that sets this (Section_UI.h's own comment on the field).
    options.bDoubleClickToggle = true;
    return options;
}
```

(`MarkersTab_Links_UI.h:77-81`.)

### Design decision 3 — the select-all pure function

New declaration in `MarkersTab_Links_UI.h` (near `DrawMarkerLinkBody`, mirrors
`DeleteMarkerLink`/`ApplyAddLinkAction`'s own "pure Apply function, declared for direct test access"
convention):

```cpp
// STEP258 — the Link header's own single-click action (LinkSectionHeaderOptions' opt-in
// bDoubleClickToggle): replaces the WHOLE selection with every Manual Instance currently tagged
// `transform.linkIdentifier == linkIdentifier`, generalizing ApplyManualInstanceSelectionClick's own
// "plain click: replace the set with just this one; the anchor becomes this one too" contract
// (MarkersTab_ManualInstanceSelection_UI.h) from one instance to a whole Link's membership. The
// primary/anchor become the FIRST instanceIdentifier encountered in `markers`' own group/transform
// walk order — the SAME deterministic order DeleteMarkerLink/PartitionLinkedManualInstancesByType
// already walk. This is a documented, arbitrary pick: no member of a Link is privileged over
// another: "the first one walked" is simply a concrete value so primary/anchor are never left
// dangling on a non-empty membership. An empty membership (a Link tagging nothing, e.g. right after
// every one of its instances was individually moved elsewhere) clears the selection entirely and
// resets both primary and anchor to -1 — "nothing to select" is not treated as "leave the OLD
// selection in place," matching Constitution §6.
void SelectAllLinkedManualInstances(const std::vector<Params::MarkerInstanceGroup>& markers,
                                    int linkIdentifier, int& selectedManualInstanceIdentifier,
                                    std::vector<int>& selectedManualInstanceIdentifiers,
                                    int& manualInstanceSelectionAnchorIdentifier);
```

Definition, `MarkersTab_Links_UI.cpp` (pure, imgui-free — placed beside `DeleteMarkerLink`, which is
already pure logic in this same file despite the file's own `#include "imgui.h"` for the outer loop):

```cpp
void SelectAllLinkedManualInstances(const std::vector<Params::MarkerInstanceGroup>& markers,
                                    int linkIdentifier, int& selectedManualInstanceIdentifier,
                                    std::vector<int>& selectedManualInstanceIdentifiers,
                                    int& manualInstanceSelectionAnchorIdentifier) {
    selectedManualInstanceIdentifiers.clear();
    for (const Params::MarkerInstanceGroup& group : markers)
        for (const Params::MarkerTransform& transform : group.transforms)
            if (transform.linkIdentifier == linkIdentifier)
                selectedManualInstanceIdentifiers.push_back(transform.instanceIdentifier);
    selectedManualInstanceIdentifier = selectedManualInstanceIdentifiers.empty()
        ? -1 : selectedManualInstanceIdentifiers.front();
    manualInstanceSelectionAnchorIdentifier = selectedManualInstanceIdentifier;
}
```

### Design decision 4 — the required companion fix: un-gate `DrawMarkerLinkHeaderExtra` from body-visibility

**This is not optional polish — without it, Part 2 silently breaks the pre-existing double-click-to-
rename feature.** Traced explicitly: `DrawMarkerLinkHeaderExtra` (rename detection AND the
`[Icon Size][Grid][SYM][V/I][LOCK][COL][swatch][X]` cluster,
`MarkersTab_LinksHeaderExtras_UI.cpp:131-175`) is currently called only *inside*
`if (DrawSectionBegin(...))` — i.e., only on frames the section is open. Today (single click always
toggles), that is harmless: double-clicking a header that starts OPEN toggles it closed then back
open across the two clicks (net unchanged), so the extra still runs on the second click's frame and
its own `ImGui::IsMouseDoubleClicked` check fires rename correctly. Under Part 2's new rule (only
click-count==2 toggles, click-count==1 does not touch `bOpen` at all), starting from OPEN (the
common case — `SectionOptions::bDefaultOpen` defaults true and nothing overrides it for Links): click
1 leaves the section open (extra runs, but click-count is 1, not 2, so no rename), click 2 toggles it
**closed** — and since the extra is gated on `bBodyVisible`, it does not run at all on that frame,
so the rename check never fires. Net: double-click-to-rename would break specifically for a header
that starts open.

**Fix:** move the `DrawMarkerLinkHeaderExtra` call outside the `if`, so it runs every frame
regardless of collapse state — restoring reliable double-click-to-rename regardless of starting
state (an unconditional call is also a strict *improvement* over today's already-latent bug where a
header starting **closed** never triggered rename at all, since in the OLD code its own
double-click-driven re-open/re-close ends CLOSED, and the extra never ran there either). Only
`DrawMarkerLinkBody` (the instance-list body) stays gated on visibility — that one genuinely should
only draw when expanded.

This introduces one side effect requiring its own one-line fix: `DrawMarkerLinkHeaderExtra`'s cluster
right-aligns itself off `ImGui::GetContentRegionAvail().x`, which shrinks once `DrawSectionBegin`'s
own `IndentSectionBody` runs (only when body-visible). Previously the extra only ever ran in the
already-indented state; now it also runs un-indented (collapsed), which would shift the cluster's
X position by one `ImGui::GetStyle().IndentSpacing` between collapsed/expanded frames. Fixed with a
temporary indent bracket matching exactly what `DrawSectionBegin`/`DrawSectionEnd`'s own default
(`options.indentWidth <= 0`) branch already does, applied only when body is NOT visible (so the two
states are never double-indented):

```cpp
if (!bBodyVisible) ImGui::Indent();   // STEP258 — match the indent DrawSectionBegin itself applies
                                       // when body-visible, so the header cluster's own right-align
                                       // math lands at the identical X whether the Link is collapsed
                                       // or expanded (the extra now runs in BOTH states, see above).
DrawMarkerLinkHeaderExtra(link, state, bAnyCommitted);
if (!bBodyVisible) ImGui::Unindent();
```

**Accepted, deliberate secondary consequence:** the Link header's own cluster (Icon Size/Grid/SYM/
V-I/LOCK/COL/Delete) is now reachable while a Link is collapsed too, not only while expanded. This
matches the Bundle tree's own established precedent one tier up — `MarkersTab_Bundles_UI.cpp:164-173`
already runs its leaf/node header-extra lambdas unconditionally, regardless of the generic tree
widget's own expand state — so this brings the Link tier in line with, not out of step with, the
tier immediately above it.

**Accepted, out-of-scope edge case (not a new regression, pre-existing ambiguity):** if a user is
mid-rename (`state.renamingLinkIdentifier == link.identifier`) and clicks elsewhere on the header
bar/InvisibleButton this same frame, `bPlainSingleClicked` still resolves and select-all can still
fire underneath the open rename textbox. The OLD code had an equivalent ambiguity (a click during
rename could also re-toggle `bOpen` underneath the textbox). Not specially suppressed here — no
report of it being a problem in the pre-existing toggle case, and inventing new suppression logic
for it is scope creep beyond what was asked.

### Full `DrawMarkerLinksSection`, after all of the above

```cpp
void DrawMarkerLinksSection(Params::MapRecipe& recipe, MarkerLinksState_UI& state,
                            int& selectedManualInstanceIdentifier,
                            std::vector<int>& selectedManualInstanceIdentifiers,
                            int& manualInstanceSelectionAnchorIdentifier,
                            const std::function<void(int, const std::vector<int>&)>&
                                selectManualMarkerInstanceCallback,
                            Pipeline::PreviewDriver* previewDriver) {
    bool bAnyLinkCommitted = false;
    for (Params::MarkerLink& link : recipe.markerLinks) {
        ImGui::PushID(link.identifier);
        bool bAnyCommitted = false;
        bool bPlainSingleClicked = false;
        const bool bBodyVisible = DrawSectionBegin(link.name.c_str(),
            state.sectionStateByLinkIdentifier[link.identifier], LinkSectionHeaderOptions(),
            LinkSectionHeaderStyle(), &bPlainSingleClicked);

        // STEP258 — a plain single click SELECTS instead of toggling (LinkSectionHeaderOptions' own
        // bDoubleClickToggle). Run before the header extra: on the SAME frame this is true, the
        // extra's own double-click-to-rename check is guaranteed false (StepSectionHeader only sets
        // bPlainSingleClicked on count==1, rename needs count==2), so there is no ordering race.
        if (bPlainSingleClicked) {
            SelectAllLinkedManualInstances(recipe.markers, link.identifier, selectedManualInstanceIdentifier,
                                           selectedManualInstanceIdentifiers, manualInstanceSelectionAnchorIdentifier);
            if (selectManualMarkerInstanceCallback)
                selectManualMarkerInstanceCallback(selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers);
        }

        // STEP258 — moved OUTSIDE the body-visible gate: see this ticket's own "Design decision 4"
        // for why gating this on bBodyVisible silently broke double-click-to-rename once the header's
        // own toggle only fires on a double-click's SECOND click. The matching indent bracket keeps
        // the cluster's own right-alignment math identical whether collapsed or expanded.
        if (!bBodyVisible) ImGui::Indent();
        DrawMarkerLinkHeaderExtra(link, state, bAnyCommitted);
        if (!bBodyVisible) ImGui::Unindent();

        if (bBodyVisible) {
            if (state.renamingLinkIdentifier != link.identifier)
                DrawMarkerLinkBody(link, recipe, selectedManualInstanceIdentifier,
                                   selectedManualInstanceIdentifiers, manualInstanceSelectionAnchorIdentifier,
                                   selectManualMarkerInstanceCallback);
            DrawSectionEnd();
        }
        bAnyLinkCommitted = bAnyLinkCommitted || bAnyCommitted;
        ImGui::PopID();
    }
    if (state.pendingDeleteLinkIdentifier >= 0) {
        DeleteMarkerLink(state.pendingDeleteLinkIdentifier, recipe.markerLinks, recipe.markers,
                         recipe.markerLayerBundles, recipe.markerLayers);
        state.pendingDeleteLinkIdentifier = -1;
    }
    NotifyPlacementChange(bAnyLinkCommitted, previewDriver);
}
```

Also add a short STEP258 addendum to the file-header comment block (`MarkersTab_Links_UI.h:1-18`),
immediately after the existing double-click-to-rename paragraph, noting: the header bar's own click
target now splits single-click (select all this Link's instances) from double-click (expand/collapse
+ still the rename trigger), and that this is the one opt-in `SectionOptions::bDoubleClickToggle`
call site in the app (`Section_UI.h`).

## Explicit out-of-scope

- No change to `WidgetStyle`, `LinkSectionHeaderStyle()`, or hover-color behavior — the color swap is
  a literal-value replacement only.
- No change to any of the 50+ other `DrawSectionBegin` call sites, or to `Section_UI_Test.cpp`'s three
  existing `StepSectionHeader` calls (verified compatible via implicit bool→int conversion).
- No delayed/debounced single-click (no `GetItemClickedCountWithSingleClickDelay`-style timer) — see
  Design decision 1's own reasoning.
- No suppression of select-all firing while a Link is mid-rename — see Design decision 4's own
  "accepted, out-of-scope edge case" note.
- No change to `MarkersTab_LinksHeaderExtras_UI.cpp` itself (`DrawMarkerLinkHeaderExtra`'s own body,
  the rename mechanism, or the button cluster) — only its call site's visibility gating changes.
- No change to `DrawMarkerLinkBody`, `PartitionLinkedManualInstancesByType`, `DeleteMarkerLink`, or
  `ApplyAddLinkAction`.
- Triple-click-and-beyond on a Link header is inert by construction (neither selects nor toggles) —
  not specially handled, not considered a defect (Design decision 1's own comment).

## Files touched

**New:** none.

**Modified:**
- `src/ui/Section_UI.h` — new `SectionOptions::bDoubleClickToggle` field, new
  `SectionChange::bPlainSingleClicked` field, widened `StepSectionHeader` signature
  (`bool bHeaderClicked` → `int headerClickCount, bool bDoubleClickToggle = false`), widened
  `DrawSectionBegin` declaration (new trailing `bool* outPlainSingleClicked = nullptr`).
- `src/ui/Section_UI.cpp` — `DrawSectionBegin` definition: click-count derivation, threaded through
  `StepSectionHeader`, out-param write.
- `src/ui/MarkersTab_Links_UI.h` — `kMarkerLinkSectionHeaderColor` value + comment (`:65-69`),
  `LinkSectionHeaderOptions()` opt-in (`:77-81`), new `SelectAllLinkedManualInstances` declaration,
  STEP258 addendum to the file-header comment block (`:1-18`).
- `src/ui/MarkersTab_Links_UI.cpp` — new `SelectAllLinkedManualInstances` definition, restructured
  `DrawMarkerLinksSection` (`:48-82`).
- `src/ui/Section_UI_Test.cpp` — new acceptance coverage (below); no CMakeLists change
  (`CMakeLists.txt:791`, already registered).
- `src/ui/MarkersTab_Links_UI_Test.cpp` — new acceptance coverage (below); no CMakeLists change
  (`CMakeLists.txt:906`, already registered).

## Acceptance tests

1. **Pure `StepSectionHeader`, default behavior pinned unchanged** (`Section_UI_Test.cpp`, no imgui
   frame) — with `bDoubleClickToggle` defaulted false: `StepSectionHeader(state, 1)` toggles exactly
   as the pre-existing `StepSectionHeader(state, true)` did; `change.bPlainSingleClicked` is false for
   every click count (0, 1, 2, 3). A direct regression pin for the "zero blast radius on every other
   header" claim.
2. **Pure `StepSectionHeader`, the new split** (`Section_UI_Test.cpp`, no imgui frame) — with
   `bDoubleClickToggle=true`: count 0 → no toggle, `bPlainSingleClicked` false; count 1 → no toggle,
   `bPlainSingleClicked` true; count 2 → toggles (state flips), `bPlainSingleClicked` false; count 3
   → no toggle, `bPlainSingleClicked` false (the "inert" case). A follow-up count 1 after the count-2
   toggle asserts `bPlainSingleClicked` true again with the state left at whatever the count-2 toggle
   set it to (proving click 1 never re-toggles).
3. **Real mouse double-click sequence through `DrawSectionBegin` itself**
   (`Section_UI_Test.cpp`, headless imgui frame, mirroring `MarkersTab_TypeSectionHideToggle_UI_Test.
   cpp`'s own multi-frame `io.AddMousePosEvent`/`io.AddMouseButtonEvent` press/release harness): settle
   the mouse over the header, single press+release (one click) — assert `outPlainSingleClicked` reads
   true and the returned body-visible bool is UNCHANGED from before the click (options carry
   `bDoubleClickToggle=true`). A second press+release immediately after (well under imgui's default
   `io.MouseDoubleClickTime`, same position) — assert on that press frame the returned body-visible
   bool has flipped (the toggle fired) and `outPlainSingleClicked` reads false. A companion pass with
   default `SectionOptions` (no `bDoubleClickToggle`) proves the FIRST single click still toggles
   immediately, live-frame-verifying Item 1's pure-logic claim end to end, not just in isolation.
4. **`SelectAllLinkedManualInstances` replaces the set** (`MarkersTab_Links_UI_Test.cpp`, pure, no
   imgui) — a `markers` fixture with instances tagged to link X, a different link, and untagged,
   plus a pre-existing, unrelated `selectedManualInstanceIdentifiers`/anchor: after the call, the
   selection is exactly the link-X-tagged identifiers in `markers`' own group/transform walk order,
   and `selectedManualInstanceIdentifier == manualInstanceSelectionAnchorIdentifier ==
   selectedManualInstanceIdentifiers.front()`.
5. **`SelectAllLinkedManualInstances` on empty membership clears, not leaves stale**
   (`MarkersTab_Links_UI_Test.cpp`, pure) — a link with zero tagged instances, called against a
   non-empty pre-existing selection: the resulting selection is empty and both primary and anchor
   reset to `-1`.
6. **Constant pins** (`MarkersTab_Links_UI_Test.cpp`, pure, one-line each) —
   `kMarkerLinkSectionHeaderColor == 0xFFFA429Eu` and `LinkSectionHeaderOptions().bDoubleClickToggle
   == true`, so either being silently reverted trips a test failure rather than only a visual/behavior
   regression a human has to notice by eye.

Full solo rebuild + `ctest -C Debug`: every previously-passing test in `Section_UI_Test` and
`MarkersTab_Links_UI_Test` stays green, plus every OTHER `*_UI_Test` binary that transitively includes
`Section_UI.h` (the whole app) compiles unchanged — a strong signal the opt-in shape is genuinely
zero-blast-radius outside `MarkersTab_Links_UI.cpp`.

// MarkersTab_Links_UI.h — STEP239/STEP241: the Markers tab's "Links" tier. Layer: UI.
// Ratifies ARCH_19_29_LinkIdentifierBackReferences.md / ARCH_19_31_PropagatedPropertyMechanisms.md
// (CORRECTED 2026-08-31 — DESIGN_MarkerLink_R1.md §3.4's correction note, STEP241). One
// `Ui::DrawSectionBegin`/`DrawSectionEnd` pair per `Params::MarkerLink` in `recipe.markerLinks`, a
// SIBLING loop to the Type-section loop (MarkersTab_UI.cpp) — never nested inside it, never folded
// into the Type-section enumeration (a Link is not a `markerTypeName` value).
//
// Mirrors the Bundle tree's own established idioms one tier over: double-click-to-rename (scratch
// buffer, `MarkersTab_BundleHeaderExtras_UI.cpp`'s own shape) committing ONLY `link.name` — STEP241
// RETRACTS STEP239's Mechanism B cascade-write into every bound Bundle's own `name`; a bound
// Bundle's/Layer's own name control is now a disabled, read-and-resolved mirror instead
// (`EffectiveMarkerLayerBundleName`/`EffectiveManualMarkerLayerName`,
// MarkersTab_MarkerLinkResolvers_UI.h), the SAME uniform mechanism every other governed field
// already uses; the Link's own full [Icon Size][Grid][SYM][V/I][LOCK][COL][swatch][X] cluster — THIS is
// the one editable surface for every governed setting while a Group/Layer is bound to it, the bound
// Layer's own controls are read-only mirrors while linked (MarkersTab_ManualLayerRowBody_UI.cpp); an
// "X" delete, deferred and applied after this loop finishes (mirrors every other pending-delete
// field in this tab, MarkersTab_Bundles_UI.h).
#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "ColorSwatch_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "PlacementRuleSections_UI.h"
#include "RtToggleWidget_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// A Link header's own [Icon Size][Grid][SYM][V/I][LOCK][COL][swatch][X] cluster — STEP241 widened
// this from STEP239's [COL][swatch][X]-only shape now that a Link governs every Section/Group-
// equivalent setting (ARCH §19.31 correction), not just color; STEP242 (ARCH §19.31's same-day
// follow-up amendment, governed field #7) adds [LOCK], mirroring the Layer tier's own built-in
// [o]/[L]/[X] affordance strip ordering (visibility, then lock). Icon Size/Grid/Symmetry/Visibility
// reuse the SAME `kMarkerLayer*` width constants the Layer tier's own header cluster already defines
// (MarkersTab_ManualLayerRowBody_UI.h) — identical UI-chrome sizing, one tier over, not a
// reinvented constant family; Color Override/Delete/Lock keep their own `kMarkerLink*` names (no
// Layer-tier equivalent exists for any of these three to share — Delete has none at the Layer tier
// at all, and Lock's own Layer-tier button is DraggableList's shared, un-exported affordance-strip
// width, not a per-control constant this file could reuse).
inline constexpr float kMarkerLinkColorOverrideButtonWidthPixels = 34.0f;
inline constexpr float kMarkerLinkColorOverrideSwatchWidthPixels = 20.0f;
inline constexpr float kMarkerLinkLockButtonWidthPixels          = 30.0f;
inline constexpr float kMarkerLinkDeleteButtonWidthPixels        = 26.0f;
inline constexpr float kMarkerLinkHeaderClusterSpacingPixels     = 8.0f;
inline constexpr float kMarkerLinkHeaderClusterWidthPixels =
    kMarkerLayerIconSizeControlWidthPixels    + kMarkerLinkHeaderClusterSpacingPixels
    + kMarkerLayerGridSizeControlWidthPixels  + kMarkerLinkHeaderClusterSpacingPixels
    + kMarkerLayerSymmetryButtonWidthPixels   + kMarkerLinkHeaderClusterSpacingPixels
    + kMarkerLayerVisibilityButtonWidthPixels + kMarkerLinkHeaderClusterSpacingPixels
    + kMarkerLinkLockButtonWidthPixels        + kMarkerLinkHeaderClusterSpacingPixels
    + kMarkerLinkColorOverrideButtonWidthPixels + kMarkerLinkHeaderClusterSpacingPixels
    + kMarkerLinkColorOverrideSwatchWidthPixels + kMarkerLinkHeaderClusterSpacingPixels
    + kMarkerLinkDeleteButtonWidthPixels;

// A named PackedColor distinct from every colorAlloy/Plasma/Spawn default and from kThemeColor's own
// resolved value (Constitution §8 — a UI-chrome tweakable, not a PARAMS/recipe value, same tier as
// kMarkerLayerHeaderExtraCombinedWidthPixels) — 0xAABBGGRR: a muted violet/rose, unlikely to collide
// visually with any marker-type tint a user configures.
inline constexpr PackedColor kMarkerLinkSectionHeaderColor = 0xFFB366CCu;

inline WidgetStyle LinkSectionHeaderStyle() {
    WidgetStyle style;
    style.trackColor = kMarkerLinkSectionHeaderColor;
    return style;
}

inline SectionOptions LinkSectionHeaderOptions() {
    SectionOptions options;
    options.reservedRightWidth = kMarkerLinkHeaderClusterWidthPixels;
    return options;
}

// Mints a fresh, never-reused Link identifier — the exact NextMarkerLayerBundleId/NextMarkerLayerId
// pattern (MarkersTab_Bundles_UI.h / MarkerLayerId_UI.h), applied to this tier.
inline int NextMarkerLinkId(const std::vector<Params::MarkerLink>& links) {
    int maximumId = -1;
    for (const Params::MarkerLink& link : links) maximumId = std::max(maximumId, link.identifier);
    return maximumId + 1;
}

struct MarkerLinksState_UI {
    // Keyed by Params::MarkerLink::identifier (stable across reorder/delete), not vector position —
    // mirrors MarkerTypeSectionsState's own string-keyed "default on first sight" contract
    // (MarkersTab_TypeSections_UI.h), one tier over.
    std::unordered_map<int, SectionState> sectionStateByLinkIdentifier;

    // Double-click-to-rename — mirrors MarkerLayerBundlesState's own renamingBundleIdentifier/
    // renameScratchText/bRenameFocusPending trio exactly, one tier up (MarkersTab_Bundles_UI.h).
    int         renamingLinkIdentifier = -1;
    std::string renameScratchText;
    bool        bRenameFocusPending    = false;

    // ONE shared RealtimeToggle instance for every Link row's own color swatch — `Params::MarkerLink`
    // cannot carry one (a pure round-tripping type), the same pre-existing limitation
    // ManualMarkerLayersState::selectedLayerColorToggle already accepts one tier down.
    RealtimeToggle colorToggle{true};

    // STEP241 — the new Icon Size/Grid Snap controls' own range + RealtimeToggle, mirroring
    // ManualMarkerLayersState's own iconScaleRange/gridSnapSizeRange/selectedLayer*Toggle fields
    // one tier up exactly (same bounds, same "Params::MarkerLink cannot own a RealtimeToggle" reason).
    ScalarSliderRange iconScaleRange{ 0.1f, 10.0f, 0.0f };
    RealtimeToggle    iconScaleToggle{true};
    ScalarSliderRange gridSnapSizeRange{ 0.1f, 100.0f, 0.0f };
    RealtimeToggle    gridSnapSizeToggle{true};

    // Deferred delete — applied by DrawMarkerLinksSection itself AFTER the per-Link loop finishes,
    // never mid-loop (erasing recipe.markerLinks while iterating it would invalidate the loop) —
    // mirrors every other pending-delete field in this tab (MarkerLayerBundlesState).
    int pendingDeleteLinkIdentifier = -1;
};

// MarkersTab_LinksHeaderExtras_UI.cpp — the aspect-split sibling (ARCH §1.5, mirroring
// MarkersTab_BundleHeaderExtras_UI.cpp's own precedent one tier up): a Link's own header-extra
// (double-click-to-rename committing `link.name` only, STEP241 — see CommitMarkerLinkRename below —
// plus the full [Icon Size][Grid][SYM][V/I][COL][swatch][X] cluster) and its read-only per-type
// instance-count summary body. Declared here so DrawMarkerLinksSection (MarkersTab_Links_UI.cpp)
// and a direct test can both drive them without an imgui frame's worth of unrelated plumbing.
void DrawMarkerLinkHeaderExtra(Params::MarkerLink& link, MarkerLinksState_UI& state, bool& bAnyCommitted);

// The rename COMMIT step only, pulled out as a pure function (mirrors DeleteMarkerLink/
// ApplyAddLinkAction's own "pure Apply function" split) so a test can drive it directly, without
// simulating imgui's own double-click/InputText focus machinery.
// STEP241/ARCH §19.31 correction — RETRACTS STEP239's Mechanism B: this used to also cascade-write
// the SAME name into every Bundle currently tagged `linkIdentifier == link.identifier`. That cascade
// is gone; a bound Bundle's/Layer's own `name` is now a disabled, read-and-resolved mirror instead
// (EffectiveMarkerLayerBundleName/EffectiveManualMarkerLayerName, MarkersTab_MarkerLinkResolvers_UI.h)
// — there is nothing left to write into any other struct on a Link rename, only the Link's own field.
inline void CommitMarkerLinkRename(Params::MarkerLink& link, const std::string& newName) {
    link.name = newName;
}
// STEP248 — replaces the old read-only DrawMarkerLinkSummaryBody with a real hierarchical body: a
// plain text label per represented Marker Type (from PartitionLinkedManualInstancesByType, NOT a
// nested Section widget — no per-type settings/collapse state of its own), followed by that type's
// Link-tagged instances rendered through the SAME rowOrder/ManualInstanceRowInteractionContext_UI/
// DrawSymmetryClusterInstanceList/DrawManualInstanceRow block DrawBaseSectionManualInstanceList
// (MarkersTab_UI.cpp, the base "Instances" list) already uses — copied verbatim one tier over, item
// source swapped only. Selection-sync (click here, highlight everywhere else) is free: same three
// shared state fields, same callback, nothing new to wire.
void DrawMarkerLinkBody(const Params::MarkerLink& link, Params::MapRecipe& recipe,
                        int& selectedManualInstanceIdentifier,
                        std::vector<int>& selectedManualInstanceIdentifiers,
                        int& manualInstanceSelectionAnchorIdentifier,
                        const std::function<void(int clickedInstanceIdentifier,
                                                 const std::vector<int>& selectedInstanceIdentifiers)>&
                            selectManualMarkerInstanceCallback);

// STEP247/ARCH §19.33 (revises STEP239's original shape): the PRIMARY walk clears
// `transform.linkIdentifier` to -1 for every MarkerTransform across `markers` matching
// `linkIdentifier` — the only tier "+Link" writes to any more. The two Bundle/Layer-tier walks that
// follow are KEPT, not removed: dead-write/live-read backward compat for any `.sanmap` still
// carrying pre-correction Layer-exclusive Link data (ARCH §19.33's explicit backward-compat ruling —
// no migration, no special-casing; they are simply a no-op for every Link this ticket's own
// `ApplyAddLinkAction` mints going forward, while staying load-bearing for legacy data). No
// instance/Layer/Group is ever erased by this action — only the tag is cleared. Erase the
// `Params::MarkerLink` entry itself LAST. Declared here (not file-local) so a test can drive it
// directly, mirroring MarkersTab_BundleDelete_UI.h's own public-pure-delete-function convention.
void DeleteMarkerLink(int linkIdentifier, std::vector<Params::MarkerLink>& links,
                      std::vector<Params::MarkerInstanceGroup>& markers,
                      std::vector<Params::MarkerLayerBundle>& bundles,
                      std::vector<Params::MarkerInstanceLayer>& markerLayers);

// STEP247/ARCH §19.33 (revises STEP239's original shape): the "+Link" button's own composed action —
// mint a `Params::MarkerLink`, then tag `MarkerTransform::linkIdentifier` DIRECTLY on every selected
// instance (MarkersTab_ManualInstanceSelection_UI.h's TagManualInstancesWithLink). No
// `Params::MarkerLayerBundle`/`Params::MarkerInstanceLayer` is minted, no
// ReassignManualInstanceLayers call — existing layering/grouping stays completely untouched.
// No-op guard, ARCH §19.33, direct human ruling: if ANY selected instance already belongs to ANY
// existing Link (IsAnyManualInstanceSelectionAlreadyLinked), this function does nothing at all — no
// new Link, no tagging — since proceeding would silently break that instance's existing Link
// membership. Also a true no-op on an empty selection (Constitution §6, never trust a caller),
// mirroring the button's own disabled-while-empty gate. Declared here (not inline in
// MarkersTab_UI.cpp's own button handler) so a test can drive it directly without an imgui frame,
// mirroring ApplyPendingCreateLayerForBundle's own "pure Apply function" split
// (MarkersTab_Bundles_UI.h).
void ApplyAddLinkAction(Params::MapRecipe& recipe, const std::vector<int>& selectedManualInstanceIdentifiers);

// The Links tier's own outer loop — one DrawSectionBegin/End pair per recipe.markerLinks entry.
// STEP248 — widened to take the shared selection state (already-live locals/parameters at
// DrawMarkersTab's relocated call site — MarkersTab_UI.h's own DrawManualMarkerLayerListBody
// parameter set, one call up the tree) so DrawMarkerLinkBody's own instance rows get the SAME
// full Ctrl/Shift/drag selection every other instance list in this tab already has, plus
// `previewDriver` so this function can call NotifyPlacementChange itself (a pre-existing,
// independent defect this ticket fixes at the same call site: bAnyCommitted was computed per Link
// and dropped, so toggling a Link's Hidden/Locked/Color/Grid/Symmetry controls never tripped a
// dirty-flag/regenerate).
void DrawMarkerLinksSection(Params::MapRecipe& recipe, MarkerLinksState_UI& state,
                            int& selectedManualInstanceIdentifier,
                            std::vector<int>& selectedManualInstanceIdentifiers,
                            int& manualInstanceSelectionAnchorIdentifier,
                            const std::function<void(int clickedInstanceIdentifier,
                                                     const std::vector<int>& selectedInstanceIdentifiers)>&
                                selectManualMarkerInstanceCallback,
                            Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen

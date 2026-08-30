// MapCanvas_IconLayer_CullInternal_UI.h — declarations shared by the three
// MapCanvas_IconLayer_Cull*_UI.cpp translation units ONLY (§1's per-layer culling + LOD split
// further than the ticket's 5-file minimum to stay inside Constitution §1.5's ceilings). Not part
// of this module's public surface (MapCanvas_IconLayer_UI.h); nothing outside this trio includes
// it. Pure, imgui-free, headless-testable — same posture as the public header.
#pragma once
#include <unordered_map>
#include "MapCanvas_IconLayer_Ops_UI.h"
#include "OverlayLayer_Settings_UI.h"
#include "../data/PlacementInstances_DATA.h"
#include "../data/PlacementResults_DATA.h"
#include "../data/RuleBucketIndexSet_DATA.h"
#include "../params/MarkerRule_PARAMS.h"
#include "../params/GlobalMarkerSettings_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"   // STEP114 — MarkerTransform/MarkerInstanceGroup

namespace SanmapGen {
namespace Ui {

struct ViewWorldRect_UI { float lowWorldX = 0.0f, lowWorldZ = 0.0f, highWorldX = 0.0f, highWorldZ = 0.0f; };

// The three top-level orchestration entry points (MapCanvas_IconLayer_Cull_UI.cpp). Declared here,
// not in the public header, because only MapCanvas_IconLayer_Draw_UI.cpp (this module's own
// imgui-facing translation unit) and this module's own tests call them directly.
ViewWorldRect_UI ComputeViewWorldRect(const PreviewComposite& composite, const MapCanvasView& view,
                                       float regionSidePixels);
// Never touches IconLayerCullDiagnostics_UI — cache-warming is not a per-frame cost to count.
void EnsureLayerAabbCache(const DrawOverlayIconLayersInput& input, IconLayerAabbCache_UI& aabbCache);
void ResolveVisibleCandidates(const DrawOverlayIconLayersInput& input, IconLayerAabbCache_UI& aabbCache,
                               IconLayerCullDiagnostics_UI* diagnostics,
                               std::vector<OverlayVisibleInstance>& outCandidates);

// §4's "run steps 1-3 fresh for only the selected instance(s)" replay-frame path — a cheap,
// picker-scoped lookup (today: Markers only, STEP48), never an O(N) re-walk of every candidate.
// STEP229 — widened from "at most one" (the old single-primary-key contract) to "one per
// Markers-collection key in the whole `selectedInstanceKeys` set" (ARCH §21.1): appends zero, one, or
// many instances. A Props/Decals key in the set is still skipped (no picker yet, STEP48, unchanged
// restriction) exactly as a Props/Decals PRIMARY already was, silently, before this ticket. Returns
// true iff at least one key resolved.
bool ResolveSelectedInstanceCandidate(const DrawOverlayIconLayersInput& input,
                                       std::vector<OverlayVisibleInstance>& outCandidates);

// Fixed 8-char (7 + NUL) tpId buffer -> std::string, stopping at the first NUL (Data::TemplateIdentifier
// / Params::UnitTransform::templateIdentifier's shared shape). No existing free helper does this
// conversion outside UI (IO must not depend upward on UI, Constitution §1) so it lives here.
std::string TemplateIdentifierToString8(const char* characters);

// §14.6: domainKind is not DATA-bucket identity. Alloy/SpawnsArmies both resolve to Markers, and
// Props/Reclaim both resolve to Props (their splits already happened at OverlayLayer seed time —
// each rule's ruleIndex was routed into exactly one of the two layers' subLayers by
// Application_OverlaySetup_Seed_UI.cpp's SeedMarkerDomains/SeedPropReclaimDomains, so no live
// per-instance category/reclaim re-check is needed here — STEP83 §0/§5).
bool TryResolveDomainCollection(OverlayDomainKind_UI domainKind, PlacementCollectionKind_UI& outCollection);
const Data::PlacementInstances& CollectionInstances(const Data::PlacementResults& placements,
                                                     PlacementCollectionKind_UI collection);
const Data::RuleBucketIndex& CollectionRuleBucket(const Data::RuleBucketIndexSet& ruleBucketIndex,
                                                   PlacementCollectionKind_UI collection);

bool WorldRectsIntersect(const LayerWorldAabb_UI& aabb, const ViewWorldRect_UI& viewRect);
void WidenAabb(LayerWorldAabb_UI& aabb, float worldX, float worldZ);

// World -> screen projection, two-mode LOD (§14.3 verbatim), pairing-lookup resolution (a miss
// draws nothing, logged at most once per unique id/session), opacity-into-tint (§14.2). Called only
// for an instance already known to be inside the view rect (the caller's per-instance AABB test).
// `bManual` (ARCH §19.25) tags the emitted OverlayInstanceKey_UI — false (default) for every
// existing procedural call site, byte-identical to before this flag existed; true only for the
// manual-marker resolver, whose `instanceIndex` is a MarkerTransform::instanceIdentifier, not a
// procedural array position.
void EmitCandidateIfVisible(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                             int layerIndex, const std::string& templateIdentifier,
                             float worldX, float worldZ, float instanceScale,
                             PlacementCollectionKind_UI collection, std::int32_t instanceIndex,
                             float tintColorRed, float tintColorGreen, float tintColorBlue,
                             int* stableOrderCounter, IconLayerCullDiagnostics_UI* diagnostics,
                             std::vector<OverlayVisibleInstance>& outCandidates, bool bManual = false);

// Category -> RGB (Alloys/Spawn resolve from GlobalMarkerSettings; Generic/Expansion/Plasma-less
// categories stay white) — UI-owned resolution of this PARAMS enum, mirroring MarkerCategoryLabel's
// own precedent (MarkersTab_Rules_UI.h). STEP111.
void ResolveMarkerCategoryTintColor(Params::MarkerCategory category,
                                    const Params::GlobalMarkerSettings& settings,
                                    float& outRed, float& outGreen, float& outBlue);

// STEP122: category -> scale multiplier (Alloys/Spawn resolve from GlobalMarkerSettings; other
// categories, including the pre-existing Plasma gap, stay a 1.0f no-op). Mirrors
// ResolveMarkerCategoryTintColor's own posture, declared beside it.
float ResolveMarkerCategoryScale(Params::MarkerCategory category, const Params::GlobalMarkerSettings& settings);

// STEP114 §4a — a manual marker's icon templateIdentifier: override wins if set, else the owning
// group's name maps to the matching GlobalMarkerSettings field, else the raw group name (v1
// Widget_MapCanvas.cpp:341 precedent). Declared here (not anonymous) so it is directly unit-
// testable, mirroring ResolveMarkerCategoryTintColor's own posture above.
std::string ResolveMarkerIconTemplateIdentifier(const Params::MarkerTransform& transform,
                                                const Params::MarkerInstanceGroup& group,
                                                const Params::GlobalMarkerSettings& globalMarkerSettings);

// `viewRect == nullptr` is the AABB-cache-build pass: only *outAabb gets widened, per instance,
// no pairing lookup / projection / emission (cheap; this is the "membership changed" rebuild, not
// a per-frame cost) and `diagnostics` must be nullptr from that caller so the query-call counter
// stays a per-FRAME count, not a cache-warm count. `viewRect != nullptr` is the real per-frame
// walk: *outAabb is left untouched (nullptr) and the per-instance AABB test against `*viewRect`
// gates whether EmitCandidateIfVisible runs at all.
//
// STEP133 — the procedural gate's own ruleIndex -> markerTypeName lookup: mirrors SeedMarkerDomains's
// existing flat-index walk shape (Application_OverlaySetup_Seed_UI.cpp's `flatIndex` counter,
// incremented once per rule across every markerRuleLayers[*].rules in order) but keeps the FULL
// `MarkerRuleLayer::markerTypeName` rather than collapsing to a Spawn/non-Spawn bool the way
// SeedMarkerDomains does — MarkerCategory alone cannot disambiguate Alloy from Plasma (no `Plasma`
// enumerator exists). Produces the SAME numbering as ProceduralInstanceRuleIndex_UI.h's
// `FlatMarkerRuleIndexBase` (STEP132) — confirmed by construction: both walk layers-then-rules in
// order, incrementing once per rule regardless of category/bEnabled/bHidden. Rebuilt fresh on every
// call, never persisted — the same zero-dirty-hash-participation posture
// ProceduralInstanceRuleIndex_UI.h's own header comment documents for its sibling index. Declared
// here (not anonymous) so this trio's own acceptance suite can exercise it directly, mirroring
// ResolveMarkerIconTemplateIdentifier's own posture.
std::unordered_map<int, std::string> BuildMarkerRuleTypeNameLookup(
    const std::vector<Params::MarkerRuleLayer>& markerRuleLayers);

// §1 item 3 — one recipe.*Rules[i]-resolved sub-layer, walked via STEP50's ruleIndex CSR bucket
// (MapCanvas_IconLayer_CullProcedural_UI.cpp).
void ResolveProceduralSubLayer(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                                int layerIndex, PlacementCollectionKind_UI collection, int ruleIndex,
                                int* stableOrderCounter, LayerWorldAabb_UI* outAabb,
                                const ViewWorldRect_UI* viewRect,
                                IconLayerCullDiagnostics_UI* diagnostics,
                                std::vector<OverlayVisibleInstance>& outCandidates);

// §1 item 4 — one hand-authored sub-layer (Props/Reclaim/Decals/Units, plus Alloy/SpawnsArmies as
// of STEP114's manual-marker icon resolver) (MapCanvas_IconLayer_CullManual_UI.cpp).
void ResolveManualSubLayer(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                            int layerIndex, int subLayerArrayIndex,
                            int* stableOrderCounter, LayerWorldAabb_UI* outAabb,
                            const ViewWorldRect_UI* viewRect,
                            IconLayerCullDiagnostics_UI* diagnostics,
                            std::vector<OverlayVisibleInstance>& outCandidates);

// ARCH §19.25 — declared here (not anonymous-namespace-local, unlike its Units/Props/Decals
// siblings) so MapCanvas_IconLayer_Cull_UI.cpp's ResolveSelectedInstanceCandidate can reuse it for
// the C2 cache's replay-frame path (§4): one manual marker sub-layer's candidates (Alloy/
// SpawnsArmies), keyed with `bManual=true` and `transform.instanceIdentifier` (never the per-group
// `index` its Units/Props/Decals siblings still use — the fix this ticket ratifies). When
// `targetInstanceIdentifier` is non-null, every transform whose own instanceIdentifier does not
// match is skipped — a scoped single-instance resolve, not a behavior change to the existing
// (`targetInstanceIdentifier == nullptr`) full-walk callers.
void ResolveMarkersManual(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                          int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                          LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                          IconLayerCullDiagnostics_UI* diagnostics,
                          std::vector<OverlayVisibleInstance>& outCandidates,
                          const int* targetInstanceIdentifier = nullptr);

} // namespace Ui
} // namespace SanmapGen

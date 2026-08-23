// MarkerOrbitCorrespondence_UI.h — the one-shot "which sibling is which orbit slot" matcher behind
// STEP94's drag-and-follow. Layer: UI. Pure, imgui-free, header-only-declared (implementation in
// the sibling .cpp) — split out of MarkerDragGesture_UI.h purely to keep that header under the
// ARCH_01_05_FileSizeCeilings.md §1.5 soft-100 ceiling, mirroring MarkerLayerIndexRepair_UI.h's own
// split off MarkersTab_ManualLayers_UI.h.
//
// R2 §1's "gesture-start proof" establishes EXACT-value equality only at the moment a gesture
// begins (an unmoved point cloud). This ticket's own acceptance test 3 (a two-bit mask, e.g.
// MirrorAcrossX|MirrorAcrossZ, growing from an on-axis to an off-axis orbit) demonstrates that
// `Proc::BuildSymmetryOrbit`'s FLAT OUTPUT INDEX for an already-tracked sibling can shift once a
// new point is inserted ahead of it in the fixed mirror/rotate/radial application order (a
// duplicate that stops colliding is spliced in wherever that pass appends, not at the array's
// tail) — so a raw frozen "orbitSlot" index recorded once at gesture-start is NOT safe to re-read
// every later frame. `MatchCorrespondenceToOrbit` instead RE-RESOLVES, every time it is called,
// which orbit slot best continues each entry's last known position — global nearest-pair-first
// greedy assignment, not per-entry independent nearest (a per-entry-independent nearest search
// could let two entries fight over the same best slot and leave a worse pairing for both). This is
// deliberately NOT the "rotated point cloud" re-identification R1 was flagged as unsafe for (R2 §1):
// that danger is about re-establishing identity from scratch against a CLOUD THAT HAS ALREADY
// ROTATED FAR from its reference; here every call anchors against each entry's own small-per-frame
// reference position, refreshed every accepted frame, so the search distance stays a single
// interactive step, never a whole gesture's worth of drift.
#pragma once
#include <vector>
#include "../pipeline/SymmetryOrbitQuery_PIPELINE.h"

namespace SanmapGen {
namespace Ui {

// One tracked sibling: which `MarkerTransform` it is, the world point it is expected near this
// call (refreshed after every frame that actually wrote it), and where `MatchCorrespondenceToOrbit`
// last resolved it to (-1 = unmatched this call, i.e. soft-hidden/collapsed).
struct MarkerOrbitCorrespondence {
    int   transformIndex      = -1;  // index into recipe.markers[groupIndex].transforms
    float referenceWorldX     = 0.0f;
    float referenceWorldZ     = 0.0f;
    int   lastMatchedOrbitSlot = -1; // index into the orbit array passed to the most recent match
    bool  bSoftHidden          = false; // this frame only; never written to recipe.markers
};

// Matches every `correspondence` entry to the nearest still-unclaimed slot in
// `orbitPoints[0, orbitCount)` (slot 0, the dragged member's own seed point, is never a candidate).
// Global greedy: every (entry, slot) pair is distance-ranked once, then claimed smallest-first, so
// a genuine continuation never loses its slot to a coincidentally-closer but wrong candidate.
// `outUnclaimedSlots`, when given, collects every slot index (other than 0) no entry claimed — the
// growth/ghost candidates. Returns how many entries were left unmatched (their `lastMatchedOrbitSlot`
// is set to -1) — the collapse/soft-hide candidates. Safe against an empty `correspondence` or an
// `orbitCount` of 0 or 1 (no siblings to claim anything).
int MatchCorrespondenceToOrbit(std::vector<MarkerOrbitCorrespondence>& correspondence,
                               const Pipeline::WorldSymmetryOrbitPoint* orbitPoints, int orbitCount,
                               std::vector<int>* outUnclaimedSlots);

} // namespace Ui
} // namespace SanmapGen

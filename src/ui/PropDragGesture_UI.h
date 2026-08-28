// PropDragGesture_UI.h — the Props-domain instantiation of the generic drag-gesture substrate
// (InstanceDragGesture_UI.h, ARCH §21.3), mirroring MarkerDragGesture_UI.h's own MarkerDragTraits.
// Gated: buildable only once §21.4's PropTransform::instanceIdentifier/symmetryGroupIdentifier
// fields exist (STEP165, already shipped) and PropsTab_ManualLayerHelpers_UI.h's
// QuantizePropPositionToLayerGrid/ResolveEffectivePropSymmetry exist (this ticket) — both true here.
// Layer: UI. Pure, imgui-free, header-only, no state of its own; not yet wired to any canvas call
// site (that lands with ARCH §21.2's gesture-ownership rewrite) — ships ahead of its own consumer,
// the same posture ARCH §20.1-§20.3/§20.6/§21.4 already establish for PARAMS/IO landing before the
// UI that reads it.
#pragma once
#include <vector>
#include "InstanceDragGesture_UI.h"
#include "PropsTab_Manual_UI.h"                  // IsPropInstanceLayerLocked
#include "PropsTab_ManualLayerHelpers_UI.h"
#include "../params/PropInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct PropDragTraits {
    using Group     = Params::PropInstanceGroup;
    using Transform  = Params::PropTransform;
    using Layer      = Params::PropInstanceLayer;

    static Group* SelectedGroup(std::vector<Group>& props, int groupIndex) {
        return SelectedPropGroup(props, groupIndex);
    }
    static Transform* SelectedInstance(std::vector<Transform>& transforms, int transformIndex) {
        return SelectedPropInstance(transforms, transformIndex);
    }
    static bool IsInstanceLayerLocked(const std::vector<Layer>& layers, int layerIndex) {
        return IsPropInstanceLayerLocked(layers, layerIndex);
    }
    static void QuantizePositionToLayerGrid(const std::vector<Layer>& layers, int layerIndex,
                                            float& x, float& z) {
        QuantizePropPositionToLayerGrid(layers, layerIndex, x, z);
    }
    static void ResolveEffectiveSymmetry(const std::vector<Layer>& layers, int layerIndex,
                                         int globalMask, int globalRadialCount, int& outMask,
                                         int& outRadialCount) {
        ResolveEffectivePropSymmetry(layers, layerIndex, globalMask, globalRadialCount, outMask, outRadialCount);
    }
    // Props has no reserved, cardinality-frozen group (unlike Markers' Spawn roster) — always false.
    static bool IsCardinalityFrozenGroup(const Group&) { return false; }
    static int  NextInstanceIdentifier(const std::vector<Group>& props) {
        return NextPropInstanceIdentifier(props);
    }
    // ARCH §21.3 refinement 2 — PropTransform carries no `name` field at all: both hooks are inert
    // no-ops, never called with anything to assign (confirmed by direct read, PropInstance_PARAMS.h).
    static void SeedInstanceName(Transform&, int) {}
    static void MakeInstanceNamesUnique(std::vector<Transform>&) {}
};

} // namespace Ui
} // namespace SanmapGen

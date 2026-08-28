// DecalDragGesture_UI.h — the Decals-domain instantiation of the generic drag-gesture substrate
// (InstanceDragGesture_UI.h, ARCH §21.3), mirroring PropDragGesture_UI.h one tier over. Layer: UI.
// Pure, imgui-free, header-only, no state of its own; not yet wired to any canvas call site (ARCH
// §21.2 lands that).
#pragma once
#include <vector>
#include "DecalsTab_Manual_UI.h"                 // IsDecalInstanceLayerLocked
#include "DecalsTab_ManualLayerHelpers_UI.h"
#include "InstanceDragGesture_UI.h"
#include "../params/PropInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct DecalDragTraits {
    using Group     = Params::DecalInstanceGroup;
    using Transform  = Params::DecalTransform;
    using Layer      = Params::DecalInstanceLayer;

    static Group* SelectedGroup(std::vector<Group>& decals, int groupIndex) {
        return SelectedDecalGroup(decals, groupIndex);
    }
    static Transform* SelectedInstance(std::vector<Transform>& transforms, int transformIndex) {
        return SelectedDecalInstance(transforms, transformIndex);
    }
    static bool IsInstanceLayerLocked(const std::vector<Layer>& layers, int layerIndex) {
        return IsDecalInstanceLayerLocked(layers, layerIndex);
    }
    static void QuantizePositionToLayerGrid(const std::vector<Layer>& layers, int layerIndex,
                                            float& x, float& z) {
        QuantizeDecalPositionToLayerGrid(layers, layerIndex, x, z);
    }
    static void ResolveEffectiveSymmetry(const std::vector<Layer>& layers, int layerIndex,
                                         int globalMask, int globalRadialCount, int& outMask,
                                         int& outRadialCount) {
        ResolveEffectiveDecalSymmetry(layers, layerIndex, globalMask, globalRadialCount, outMask, outRadialCount);
    }
    // Decals has no reserved, cardinality-frozen group — always false.
    static bool IsCardinalityFrozenGroup(const Group&) { return false; }
    static int  NextInstanceIdentifier(const std::vector<Group>& decals) {
        return NextDecalInstanceIdentifier(decals);
    }
    // ARCH §21.3 refinement 2 — DecalTransform carries no `name` field either: inert no-ops.
    static void SeedInstanceName(Transform&, int) {}
    static void MakeInstanceNamesUnique(std::vector<Transform>&) {}
};

} // namespace Ui
} // namespace SanmapGen

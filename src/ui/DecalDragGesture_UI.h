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
    // ARCH §21.9 — Decals has no Link concept: `using Link = NoInstanceLink` and every widened method
    // below is an inert pass-through, ignoring `links` and reading `transform.layerIndex` into the
    // SAME unchanged, still-2-parameter free functions this file always called.
    using Link = NoInstanceLink;

    static bool IsInstanceEffectivelyLocked(const std::vector<Layer>& layers, const Transform& transform,
                                            const std::vector<Link>&) {
        return IsDecalInstanceLayerLocked(layers, transform.layerIndex);
    }
    static void QuantizePositionToLayerGrid(const std::vector<Layer>& layers, const Transform& transform,
                                            const std::vector<Link>&, float& x, float& z) {
        QuantizeDecalPositionToLayerGrid(layers, transform.layerIndex, x, z);
    }
    static void ResolveEffectiveSymmetry(const std::vector<Layer>& layers, const Transform& transform,
                                         const std::vector<Link>&, int globalMask, int globalRadialCount,
                                         int& outMask, int& outRadialCount) {
        ResolveEffectiveDecalSymmetry(layers, transform.layerIndex, globalMask, globalRadialCount,
                                      outMask, outRadialCount);
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

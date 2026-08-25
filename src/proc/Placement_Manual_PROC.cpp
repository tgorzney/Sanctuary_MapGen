// Placement_Manual_PROC.cpp — hand-authored props/decals resolution. Straight 1:1 copy-through
// of `recipe.props`/`recipe.decals` into `results.props`/`results.decals`: no symmetry-orbit
// expansion, no terrain resampling, no field is computed or reinterpreted (ARCH_14_13_OpenItems.md
// §14.13 item 3, "WORK-ORDER B", Ruling 3). Mirrors the `recipe.armies`/`recipe.markers`
// precedent: hand-placed data's entire purpose is round-trip fidelity, so no PROC stage
// touches it beyond copying it into the resolved SoA. CPU only, unconditionally — this is a
// data copy, not a compute kernel; no GPU twin exists or is authorized (see the work-order's
// "Backend policy").
#include "Placement_PROC.h"

namespace SanmapGen {
namespace Proc {
namespace {

Data::PlacementInstance MakeManualInstance(const Params::InstancedTransform& transform) {
    Data::PlacementInstance instance;   // every other field stays at its own default (see ruling)
    instance.positionX = transform.positionX; instance.positionY = transform.positionY;
    instance.positionZ = transform.positionZ;
    instance.rotationX = transform.rotationX; instance.rotationY = transform.rotationY;
    instance.rotationZ = transform.rotationZ; instance.rotationW = transform.rotationW;
    instance.scaleX = transform.scaleX; instance.scaleY = transform.scaleY; instance.scaleZ = transform.scaleZ;
    instance.ruleIndex = -1;   // NOT the struct default (0) — 0 is a live procedural rule index;
                               // -1 is the sentinel STEP50's CSR bucket build excludes (STEP83 §7)
    return instance;
}

} // namespace

void PlacementStage::ResolveManualPropsAndDecals() {
    for (const Params::PropInstanceGroup& group : recipe.props) {
        for (const Params::PropTransform& transform : group.transforms) {
            Data::PlacementInstance instance = MakeManualInstance(transform.transform);
            instance.manualLayerId = Params::ResolvePropInstanceLayerId(transform.layerIndex, recipe.propLayers);
            results.props.Append(instance);
        }
    }
    for (const Params::DecalInstanceGroup& group : recipe.decals) {
        for (const Params::DecalTransform& transform : group.transforms) {
            Data::PlacementInstance instance = MakeManualInstance(transform.transform);
            instance.manualLayerId = Params::ResolveDecalInstanceLayerId(transform.layerIndex, recipe.decalLayers);
            results.decals.Append(instance);
        }
    }
}

} // namespace Proc
} // namespace SanmapGen

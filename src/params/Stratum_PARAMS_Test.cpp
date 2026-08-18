// Stratum_PARAMS_Test.cpp — tab-rebuild C2 acceptance, PARAMS half: `Params::Stratum` is the ONE
// per-stratum settings type (ARCH §7.1) and it now reaches the soil physics and the material
// appearance the Stratums tab edits. Pure checks; no imgui, no window, no GL context.
#include "Stratum_PARAMS.h"
#include "MapRecipe_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// The composition ARCH §7.1 allows: sub-structs REACHED THROUGH Params::Stratum, never rival
// top-level per-stratum settings types.
void RunCompositionChecks() {
    Params::Stratum stratum;
    stratum.soilPhysics.hardness  = 0.75f;
    stratum.appearance.name       = "Grass";
    Check(stratum.soilPhysics.hardness == 0.75f, "the soil physics is reached through the stratum");
    Check(stratum.appearance.name == "Grass", "so is the material appearance");

    Params::MapRecipe recipe;
    recipe.strata.resize(9);
    recipe.strata[4].soilPhysics.cohesion = 0.2f;
    Check(recipe.strata[4].soilPhysics.cohesion == 0.2f,
          "and the recipe's stratum array carries both, so both round-trip with the map");
}

// The defaults must MATCH `Proc::MaterialPhysics`, or promoting the settings home would silently
// re-tune every existing map the first time the UI pushes them onto the sim.
void RunSoilDefaultChecks() {
    Params::StratumSoilPhysics soilPhysics;
    Check(soilPhysics.hardness == 0.2f, "hardness keeps the runtime record's default");
    Check(soilPhysics.friction == 0.8f, "friction keeps the runtime record's default");
    Check(soilPhysics.cohesion == 0.5f, "cohesion keeps the runtime record's default");
    Check(soilPhysics.capacityMultiplier == 2.0f, "capacity multiplier keeps its default");
    Check(soilPhysics.absorptionRate == 0.01f, "absorption rate keeps its default");
    Check(soilPhysics.bErodable, "a stratum is erodable until told otherwise");
}

// The appearance defaults are the identity: a stratum nobody has touched must bake exactly as it
// did before the fields existed.
void RunAppearanceDefaultChecks() {
    Params::StratumAppearance appearance;
    Check(appearance.name.empty() && appearance.environmentName.empty()
          && appearance.materialName.empty(), "an untouched stratum names nothing");
    Check(appearance.albedoTexturePath.empty() && appearance.normalTexturePath.empty()
          && appearance.compositeTexturePath.empty(), "and points at no texture");
    for (int channel = 0; channel < Params::kStratumColorChannelCount; ++channel) {
        Check(appearance.diffuseRemapColor[channel] == 1.0f, "the diffuse remap defaults to white");
        Check(appearance.farColorRemapColor[channel] == 1.0f, "so does the far color remap");
    }
    Check(appearance.farTileCount == 1.0f && appearance.triplanarTileCount == 1.0f
          && appearance.farTriplanarTileCount == 1.0f, "every tile count defaults to one repeat");
    Check(appearance.normalScale == 1.0f && appearance.farNormalScale == 1.0f,
          "normal detail defaults to full scale");
    Check(appearance.normalFarNearBlend == 0.0f && appearance.heightFarNearBlend == 0.0f,
          "and the far/near crossfades default to the near end");
}

// The fields the stages already consume must be untouched by the promotion (ARCH §7.2.5: the one
// remap, the one tint, the one tile count — no second copy anywhere).
void RunExistingFieldChecks() {
    Params::Stratum stratum;
    bool bRemapIsIdentity = true;
    for (int channel = 0; channel < Params::kStratumColorChannelCount; ++channel) {
        bRemapIsIdentity &= stratum.maskRemapMinimum[channel] == 0.0f;
        bRemapIsIdentity &= stratum.maskRemapMaximum[channel] == 1.0f;
    }
    Check(bRemapIsIdentity,
          "the ONE surface-weight remap is still identity by default, now across all 4 channels "
          "(ARCH §7.2 item 10)");
    Check(stratum.tintRed == 1.0f && stratum.tintGreen == 1.0f && stratum.tintBlue == 1.0f,
          "the preview base color is still the stratum's own tint, not a second color");
    Check(stratum.tileCount == 1.0f, "the near tile count is still the stratum's own field");
    Check(stratum.importedMaskMode == Params::ImportedMaskMode::Disabled,
          "and the stored-art merge still starts disabled");
}

} // namespace

int main() {
    RunCompositionChecks();
    RunSoilDefaultChecks();
    RunAppearanceDefaultChecks();
    RunExistingFieldChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

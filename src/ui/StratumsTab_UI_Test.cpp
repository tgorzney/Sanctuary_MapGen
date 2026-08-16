// StratumsTab_UI_Test.cpp — tab-rebuild C2 acceptance, part 1: the Stratums tab's slider
// catalogue, the nine-slot palette, the borrowed sanpack dropdowns, the 3-state mask mode and the
// preview-color mirror. Pure checks driven with synthetic values — no imgui frame, no window, no GL
// context. main() lives here; the soil-physics half is StratumsTab_Soil_UI_Test.cpp.
#include "StratumsTab_TestSupport_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstring>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

void RunStratumsTabSoilChecks();          // StratumsTab_Soil_UI_Test.cpp

namespace {

// Every slider the tab offers must have a label and a usable range, or a control could be drawn
// with no bounds to clamp against.
void RunScalarCatalogueChecks() {
    for (int scalarIndex = 0; scalarIndex < kStratumsTabScalarCount; ++scalarIndex) {
        const StratumsTabScalarDescription& description =
            StratumsTabScalarDescriptionOf(static_cast<StratumsTabScalar>(scalarIndex));
        CheckStratumsTab(description.label != nullptr && description.label[0] != '\0',
                         "every catalogued stratum control is labelled");
        CheckStratumsTab(description.range.maximumValue > description.range.minimumValue,
                         "and carries a range a handle can move inside");
    }
    CheckStratumsTab(StratumsTabScalarDescriptionOf(static_cast<StratumsTabScalar>(-1)).label[0] == '\0',
                     "an out-of-range enumerator answers an empty row, never a neighbour");

    StratumsTabState state;
    CheckStratumsTab(state.scalarRanges[static_cast<int>(StratumsTabScalar::SoilHardness)].minimumValue
                     == 0.01f, "the state copies the catalogue's limits, so a host may retune them");
    CheckStratumsTab(!state.rows[0].section.bOpen
                     && !state.rows[kStratumsTabStratumCount - 1].section.bOpen,
                     "every stratum section opens closed - nine open sections is a wall");
    CheckStratumsTab(state.rows[0].soilPresetIndex == -1, "and no preset is pre-picked");
}

// The tab draws nine sections; a recipe shorter than the palette is grown, never indexed past.
void RunPaletteChecks() {
    Params::MapRecipe recipe;
    CheckStratumsTab(recipe.strata.empty(), "a fresh recipe carries no strata");
    EnsureStratumPalette(recipe.strata);
    CheckStratumsTab(static_cast<int>(recipe.strata.size()) == kStratumsTabStratumCount,
                     "the tab grows the recipe to the full palette");
    recipe.strata[3].tintRed = 0.25f;
    EnsureStratumPalette(recipe.strata);
    CheckStratumsTab(recipe.strata[3].tintRed == 0.25f, "and a second growth disturbs nothing");
    CheckStratumsTab(kStratumsTabStratumCount == Data::MapFields::stratumCount,
                     "the tab's palette width is the fields' width, never a second number");
}

// The name a dropdown row carries is what the recipe stores, so a swapped sanpack cannot silently
// re-point a stratum at a neighbouring material.
void RunAssetOptionChecks() {
    const StratumsTabAssetOptions options = MakeTestAssetOptions();
    CheckStratumsTab(StratumOptionIndexOf("Sand02", options.materialLabels, options.materialCount) == 1,
                     "a stored material name resolves onto its row");
    CheckStratumsTab(StratumOptionIndexOf("Snow09", options.materialLabels, options.materialCount) == -1,
                     "a name the pack no longer offers shows nothing picked");
    CheckStratumsTab(StratumOptionIndexOf("", options.materialLabels, options.materialCount) == -1,
                     "an unset name shows nothing picked");
    CheckStratumsTab(StratumOptionIndexOf("Grass01", nullptr, 0) == -1,
                     "and no pack loaded picks nothing");
    CheckStratumsTab(std::strcmp(StratumOptionLabelAt(2, options.materialLabels, options.materialCount),
                                 "Rock03") == 0, "a row answers its own label");
    CheckStratumsTab(StratumOptionLabelAt(9, options.materialLabels, options.materialCount)[0] == '\0',
                     "and a row the table does not carry answers nothing, never off the end");
}

// The 3-state mask mode cycles exactly as v1's button did.
void RunMaskModeChecks() {
    CheckStratumsTab(NextImportedMaskMode(Params::ImportedMaskMode::Disabled)
                     == Params::ImportedMaskMode::ProceduralStart,
                     "disabled cycles to procedural start");
    CheckStratumsTab(NextImportedMaskMode(Params::ImportedMaskMode::ProceduralStart)
                     == Params::ImportedMaskMode::StaticOverride,
                     "procedural start cycles to static override");
    CheckStratumsTab(NextImportedMaskMode(Params::ImportedMaskMode::StaticOverride)
                     == Params::ImportedMaskMode::Disabled,
                     "and static override cycles back to disabled");
    for (int modeIndex = 0; modeIndex < kImportedMaskModeCount; ++modeIndex)
        CheckStratumsTab(importedMaskModeLabels[modeIndex] != nullptr, "every mask mode is labelled");

    Params::Stratum stratum;
    char label[80] = {};
    FormatStratumSectionLabel(3, stratum, label, sizeof(label));
    CheckStratumsTab(std::strcmp(label, "Stratum 3") == 0, "an unnamed stratum still has a header");
    stratum.appearance.name = "Grass";
    FormatStratumSectionLabel(3, stratum, label, sizeof(label));
    CheckStratumsTab(std::strcmp(label, "Stratum 3 - Grass") == 0,
                     "a named one shows index and name");
}

// The preview base color is the stratum's own tint (no rival color field), edited through an RGBA
// mirror because the swatch widget speaks RGBA.
void RunColorMirrorChecks() {
    Params::Stratum stratum;
    StratumRowState row;
    stratum.tintRed = 0.2f; stratum.tintGreen = 0.4f; stratum.tintBlue = 0.6f;
    stratum.appearance.environmentName = "Desert";
    stratum.appearance.materialName    = "Rock03";
    LoadStratumRowValues(stratum, MakeTestAssetOptions(), row);
    CheckStratumsTab(row.previewBaseColorMirror[0] == 0.2f && row.previewBaseColorMirror[1] == 0.4f
                     && row.previewBaseColorMirror[2] == 0.6f,
                     "the tint loaded into the swatch mirror");
    CheckStratumsTab(row.previewBaseColorMirror[3] == 1.0f,
                     "with an opaque alpha the tint does not carry");
    CheckStratumsTab(row.environmentIndex == 1 && row.materialIndex == 2,
                     "and both dropdowns resolved");
    CheckStratumsTab(!StoreStratumRowValues(row, stratum), "an untouched round trip moves nothing");

    row.previewBaseColorMirror[1] = 0.9f;
    CheckStratumsTab(StoreStratumRowValues(row, stratum), "a moved swatch reports the recipe moved");
    CheckStratumsTab(stratum.tintGreen == 0.9f, "and it reaches the stratum's own tint");
}

} // namespace

int main() {
    RunScalarCatalogueChecks();
    RunPaletteChecks();
    RunAssetOptionChecks();
    RunMaskModeChecks();
    RunColorMirrorChecks();
    RunStratumsTabSoilChecks();
    return ReportStratumsTabTestResult();
}

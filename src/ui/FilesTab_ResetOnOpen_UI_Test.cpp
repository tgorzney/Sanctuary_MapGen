// FilesTab_ResetOnOpen_UI_Test.cpp — STEP103 acceptance: RunOpenSanmap builds fresh scratch
// recipe/fields/baked-image state and commits it onto the live objects only on success. Confirmed
// root cause this closes: Application's live `recipe` starts seeded with two default procedural
// noise layers (Application_Recipe_UI.cpp); a real .sanmap with no HeightmapStack section left them
// in place, defeating STEP101's empty-stack fresh-synthesis gate and silently leaving procedural
// noise where the imported terrain should be. Headless: no imgui frame, no window, no GL context,
// same posture as every other Files-tab test unit.
#include "Application_Defaults_UI.h"
#include "FilesTab_TestSupport_UI.h"
#include "FilesTab_UI.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../data/MapFields_DATA.h"
#include "../io/MapExporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"
#include <cmath>

namespace SanmapGen {
namespace FilesTabTest {
namespace {

bool NearlyEqual(float left, float right, float tolerance) { return std::fabs(left - right) <= tolerance; }

// A genuine externally-authored `.sanmap`: an empty layerStack (so no HeightmapStack section
// survives export), a small non-trivial heightfield, and a single fully-covering stratum-1 mask —
// the same fixture shape STEP101's own acceptance test uses, reproduced here since it is a
// different translation unit with no shared helper.
void WriteSyntheticExternalMap(const std::string& folderPath, Data::FloatField& outOriginalHeightfield) {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 8;
    const int vertexSize = recipe.geometry.VertexSize();

    Data::MapFields fields;
    fields.Resize(vertexSize, 0.0f);
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            fields.heightfield.Set(x, y, static_cast<float>((x * 5 + y * 3) % 11) / 10.0f);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        fields.surfaceStratumWeights[stratum].Fill(stratum == 1 ? 1.0f : 0.0f);

    Io::MapExportOptions exportOptions;
    const Io::MapExportResult exportResult =
        Io::MapExporter::ExportAll(folderPath, recipe, fields, exportOptions);
    Check(exportResult.bSucceeded, "the synthetic external-map fixture exports");
    outOriginalHeightfield = fields.heightfield;
}

// Acceptance test 1: the reported bug, end to end.
void CheckOpenReplacesDefaultNoiseLayersAndReproducesTheImportedTerrain() {
    const std::string folderPath = ScratchFolderPath("SanGenResetOnOpenBugFix");
    Data::FloatField originalHeightfield;
    WriteSyntheticExternalMap(folderPath, originalHeightfield);

    // Application's real startup state: MakeDefaultMapRecipe() seeds one "Terrain" GeoLayer with
    // two non-baked procedural noise layers.
    Params::MapRecipe recipe = Ui::MakeDefaultMapRecipe();
    Check(recipe.layerStack.geoLayers.size() == 1 && recipe.layerStack.TotalLayerCount() == 2,
          "the live recipe starts with the default noise layer stack");

    // assembler(recipe) binds by reference, exactly as Application_UI.cpp's own assembler does — the
    // live object Open mutates must land IN PLACE, never rebind, for this to prove anything real.
    Pipeline::GenerationAssembler assembler(recipe);

    Ui::FilesTabState state;
    state.sanmapPath = folderPath;
    Data::MapFields fields;
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::OpenSanmap, state, recipe, &fields, false,
                                &assembler.BakedLayerImages()),
          "Open succeeds on the synthetic external map");

    // STEP109: the fresh-synthesis group's name derives from the document's own FILENAME stem
    // (underscores -> spaces), never a hardcoded literal. WriteSyntheticExternalMap exports through
    // a default-constructed Params::MapRecipe, whose mapName ("mapdef", MapRecipe_PARAMS.h's own
    // default) becomes the document's filename -- "mapdef.sanmap" -- so the derived name is "mapdef"
    // verbatim (no underscore to swap).
    Check(recipe.layerStack.geoLayers.size() == 1
          && recipe.layerStack.geoLayers[0].name == "mapdef",
          "the two default noise layers are gone; exactly one fresh-synthesis group (named from the "
          "document's own filename stem) replaces them");
    bool bEveryLayerBaked = true;
    for (const Params::GeoLayer& group : recipe.layerStack.geoLayers)
        for (const Params::Layer& layer : group.layers)
            if (!layer.bBaked) bEveryLayerBaked = false;
    Check(bEveryLayerBaked, "every surviving layer is bBaked -- no leftover procedural layer");

    assembler.Thermal().Constants().iterationCount = 0;   // isolate NoiseBlend's own reproduction
    assembler.Run();
    const Data::FloatField& assembled = assembler.Fields().heightfield;
    bool bMatches = true;
    for (int y = 0; y < originalHeightfield.Height() && bMatches; ++y)
        for (int x = 0; x < originalHeightfield.Width() && bMatches; ++x)
            if (!NearlyEqual(assembled.Get(x, y), originalHeightfield.Get(x, y), 1.0e-4f)) bMatches = false;
    Check(bMatches, "assembler.Run() reproduces the imported heightfield, not procedural noise -- "
                    "the reported bug's actual fix");
}

// Acceptance test 2: BakedLayerImages no longer bleeds across files.
void CheckBakedLayerImagesDoNotBleedAcrossOpens() {
    const std::string folderPath = ScratchFolderPath("SanGenResetOnOpenBleedThrough");
    Data::FloatField originalHeightfield;
    WriteSyntheticExternalMap(folderPath, originalHeightfield);

    Params::MapRecipe recipe = Ui::MakeDefaultMapRecipe();
    Pipeline::GenerationAssembler assembler(recipe);
    // A sentinel entry, as if left by a prior Open of a DIFFERENT file — same low layerIdentifier a
    // fresh recipe's counter always restarts at.
    Data::BakedLayerImage sentinel;
    sentinel.layerIdentifier = 0;
    sentinel.image.Resize(2, 2, 0.0f);
    sentinel.image.Set(0, 0, 777.0f);
    assembler.BakedLayerImages().push_back(sentinel);

    Ui::FilesTabState state;
    state.sanmapPath = folderPath;
    Data::MapFields fields;
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::OpenSanmap, state, recipe, &fields, false,
                                &assembler.BakedLayerImages()),
          "Open succeeds");

    Check(assembler.BakedLayerImages().size() == 1,
          "the cache holds ONLY the new file's one entry (STEP105: a single flattened baked layer) "
          "-- the sentinel is gone");
    const Data::FloatField* stratum0Image = Data::FindBakedLayerImage(assembler.BakedLayerImages(), 0);
    Check(stratum0Image != nullptr, "layerIdentifier 0 still resolves to a stored image");
    Check(stratum0Image != nullptr && !NearlyEqual(stratum0Image->Get(0, 0), 777.0f, 1.0f),
          "and it is the NEW file's stratum-0 contribution, not the sentinel's frozen pixel");
}

// Acceptance test 3: a refused/failed Open never touches live state.
void CheckARefusedOpenLeavesLiveStateUntouched() {
    Params::MapRecipe recipe = Ui::MakeDefaultMapRecipe();
    recipe.mapName = "PRE_OPEN_MARKER";
    recipe.geometry.mapSize = 32;
    const int vertexSize = recipe.geometry.VertexSize();

    Data::MapFields fields;
    fields.Resize(vertexSize, 0.0f);
    fields.heightfield.Set(1, 1, 0.5f);

    std::vector<Data::BakedLayerImage> bakedLayerImages;
    Data::BakedLayerImage sentinel;
    sentinel.layerIdentifier = 5;
    sentinel.image.Resize(2, 2, 3.5f);
    bakedLayerImages.push_back(sentinel);

    Ui::FilesTabState state;
    state.sanmapPath = "D:/no/such/place/mapdef.sanmap";
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::OpenSanmap, state, recipe, &fields, false,
                                 &bakedLayerImages),
          "opening a nonexistent path is refused");

    Check(recipe.mapName == "PRE_OPEN_MARKER" && recipe.geometry.mapSize == 32
          && recipe.layerStack.geoLayers.size() == 1,
          "the live recipe is byte-identical to before the call -- no partial reset on a refusal");
    Check(fields.IsSized() && NearlyEqual(fields.heightfield.Get(1, 1), 0.5f, 1.0e-6f),
          "the live fields are untouched");
    Check(bakedLayerImages.size() == 1 && bakedLayerImages[0].layerIdentifier == 5
          && NearlyEqual(bakedLayerImages[0].image.Get(0, 0), 3.5f, 1.0e-6f),
          "and the live baked-image cache is untouched");
}

// Acceptance test 4: checkbox-off is unaffected IN KIND, only in what it now correctly shows. With
// bLoadBakedFieldsOnImport == false, LoadBakedFields never runs, so
// DecomposeBakedHeightmapIntoLayers never runs either: geoLayers stays at the fresh scratch
// recipe's own empty default -- no default noise layers, no synthesized baked layers. Expected,
// correct post-fix behaviour, not a regression.
void CheckCheckboxOffProducesAGenuinelyEmptyLayerStack() {
    const std::string folderPath = ScratchFolderPath("SanGenResetOnOpenCheckboxOff");
    Data::FloatField originalHeightfield;
    WriteSyntheticExternalMap(folderPath, originalHeightfield);

    Params::MapRecipe recipe = Ui::MakeDefaultMapRecipe();
    Ui::FilesTabState state;
    state.sanmapPath = folderPath;
    state.bLoadBakedFieldsOnImport = false;
    Data::MapFields fields;
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::OpenSanmap, state, recipe, &fields),
          "Open still succeeds with the texture toggle off");
    Check(recipe.layerStack.geoLayers.empty(),
          "the layer stack ends up genuinely empty -- no default noise, no synthesized baked layers "
          "(the checkbox explicitly opted out of texture loading; not a regression)");
    Check(!fields.IsSized(), "and no texture was read");
}

} // namespace

void RunResetOnOpenTests() {
    CheckOpenReplacesDefaultNoiseLayersAndReproducesTheImportedTerrain();
    CheckBakedLayerImagesDoNotBleedAcrossOpens();
    CheckARefusedOpenLeavesLiveStateUntouched();
    CheckCheckboxOffProducesAGenuinelyEmptyLayerStack();
}

} // namespace FilesTabTest
} // namespace SanmapGen

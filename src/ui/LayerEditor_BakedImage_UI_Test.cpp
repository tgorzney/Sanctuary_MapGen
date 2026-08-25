// LayerEditor_BakedImage_UI_Test.cpp — STEP102 acceptance: ApplyBakedImageAction wires Import RAW
// and the Bake/Unbake toggle to a real Pipeline::GenerationAssembler. Unlike the erosion checks
// (LayerEditor_Erosion_UI_Test.cpp), which only edit constants reached through the assembler, this
// drives real `Run()`s -- the whole point is observing what the NEXT run puts in
// `assembler.Fields().heightfield`. main() lives in LayerEditor_UI_Test.cpp.
#include "LayerEditor_BakedImage_UI.h"
#include "LayerEditor_TestSupport_UI.h"
#include "../io/FilesystemPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"
#include <cstdint>
#include <filesystem>
#include <vector>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

constexpr int kFixtureMapSize = 15;   // vertexSize 16 -- small and fast, matches
                                      // NoiseBlend_Baked_PROC_Test.cpp's own scale.

// A private, EMPTY scratch folder -- same posture as MapFormat_TestSupport_IO.h's own helper
// (not reused directly: that header lives in src/io and carries its own MapFormatTest::Check).
std::string ScratchFolderPath(const char* folderName) {
    std::error_code pathError;
    const std::filesystem::path folder = std::filesystem::temp_directory_path(pathError) / folderName;
    std::filesystem::remove_all(folder, pathError);
    std::filesystem::create_directories(folder, pathError);
    return folder.string();
}

// A real 16-bit LE RAW heightmap sized to the fixture map: one full-height sample at (1, 1),
// everything else at the floor.
std::string WriteFixtureRawFile(const std::string& folderPath, int vertexSize) {
    std::vector<std::uint16_t> samples(static_cast<std::size_t>(vertexSize) * vertexSize, 0);
    samples[static_cast<std::size_t>(vertexSize) + 1u] = 65535u;   // column 1, row 1
    const std::string filePath = folderPath + "/fixture.raw";
    Io::WriteBinaryFileBytes(filePath, samples.data(), samples.size() * sizeof(std::uint16_t));
    return filePath;
}

// One GeoLayer, one Layer, small geometry -- Params::MapRecipe's documented "strata past the end
// run on their defaults" posture means an empty `strata`/rule array degrades gracefully rather
// than needing to be fully populated for a real Run() to be safe.
Params::MapRecipe FixtureRecipe() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = kFixtureMapSize;
    Params::GeoLayer group;
    group.layers.push_back(Params::Layer());
    recipe.layerStack.geoLayers.push_back(group);
    return recipe;
}

void RunImportRawChecks() {
    const std::string folderPath = ScratchFolderPath("SanGenLayerEditorBakedImageTest");
    Params::MapRecipe recipe = FixtureRecipe();
    const std::string rawPath = WriteFixtureRawFile(folderPath, recipe.geometry.VertexSize());

    Pipeline::GenerationAssembler assembler(recipe);
    // Thermal relaxation is on by default and would slump a single imported spike toward its flat
    // neighbours before this test ever reads it -- disabled to isolate the mechanism under test,
    // same precedent as MapImporter_HeightmapDecomposition_IO_Test.cpp's own comment (Erosion is
    // already a no-op by default -- ErosionLayerSettings::bEnabled starts false).
    assembler.Thermal().Constants().iterationCount = 0;
    // Fields() is sized by NoiseBlend's own PrepareRun(), not by construction (GenerationAssembler
    // stays zero-sized until the first Run()) -- exactly the state the REAL app is already past by
    // the time a user reaches the Layer Editor's Import RAW button, since the assembler runs once
    // on open before any tab draws. Matched here rather than papered over.
    assembler.Run();
    Params::Layer& layer = recipe.layerStack.geoLayers[0].layers[0];

    LayerEditorAction action;
    action.kind           = LayerEditorActionKind::ImportRawRequested;
    action.geoLayerIndex  = 0;
    action.layerIndex     = 0;
    action.importRawPath  = rawPath;
    CheckLayerEditor(ApplyBakedImageAction(action, recipe.layerStack, assembler),
                     "Import RAW on a real file reports the recipe moved");
    CheckLayerEditor(layer.bBaked, "and the layer becomes baked");
    CheckLayerEditor(layer.bakedImagePath == rawPath, "with the picked path recorded");
    CheckLayerEditor(layer.layerIdentifier >= 0, "and a stable identifier is assigned");
    const Data::FloatField* image =
        Data::FindBakedLayerImage(assembler.BakedLayerImages(), layer.layerIdentifier);
    CheckLayerEditor(image != nullptr, "a matching Data::BakedLayerImage appears in BakedLayerImages()");

    assembler.Run();
    CheckLayerEditor(assembler.Fields().heightfield.Get(1, 1)
                    > assembler.Fields().heightfield.Get(0, 0) + 0.5f,
                     "the NEXT Run() reproduces the file's contents in mapFields.heightfield");
}

// A rejected/unreadable path leaves the layer untouched (Constitution §6 -- no crash, no partial
// write). The picker's own extension fence (DrawFilePathPicker) is what usually keeps a bad
// extension from reaching this far; this exercises the IO-level refusal underneath it.
void RunImportRawFailureChecks() {
    Params::MapRecipe recipe = FixtureRecipe();
    Pipeline::GenerationAssembler assembler(recipe);
    Params::Layer& layer = recipe.layerStack.geoLayers[0].layers[0];

    LayerEditorAction action;
    action.kind           = LayerEditorActionKind::ImportRawRequested;
    action.geoLayerIndex  = 0;
    action.layerIndex     = 0;
    action.importRawPath  = "C:/SanGenTest/does/not/exist.raw";
    CheckLayerEditor(!ApplyBakedImageAction(action, recipe.layerStack, assembler),
                     "Import RAW on an unreadable file reports the recipe untouched");
    CheckLayerEditor(!layer.bBaked && layer.bakedImagePath.empty() && layer.layerIdentifier == -1,
                     "and the layer's baked state stays exactly as it was, no crash");
}

void RunBakeToggleChecks() {
    Params::MapRecipe recipe = FixtureRecipe();
    Pipeline::GenerationAssembler assembler(recipe);
    Params::Layer& layer = recipe.layerStack.geoLayers[0].layers[0];

    assembler.Run();
    const float liveHeight = assembler.Fields().heightfield.Get(1, 1);

    LayerEditorAction bake;
    bake.kind          = LayerEditorActionKind::BakeToggleRequested;
    bake.geoLayerIndex  = 0;
    bake.layerIndex     = 0;
    CheckLayerEditor(ApplyBakedImageAction(bake, recipe.layerStack, assembler),
                     "Bake on a live noise layer reports the recipe moved");
    CheckLayerEditor(layer.bBaked, "and flips the layer to baked");
    CheckLayerEditor(layer.bakedImagePath.empty(),
                     "with no file path -- sourced from live noise, not a file");
    const Data::FloatField* image =
        Data::FindBakedLayerImage(assembler.BakedLayerImages(), layer.layerIdentifier);
    CheckLayerEditor(image != nullptr, "and a Data::BakedLayerImage snapshot appears");

    // STEP152: baking this one layer removes the LAST active procedural layer, so Erosion/
    // Thermal/FlowAccumulation stop running entirely on every run from here on (the ticket's own
    // ratified tradeoff) -- the frozen baseline this test compares against is therefore captured
    // AFTER the bake takes effect, not `liveHeight` above (which still had thermal relaxation
    // applied on top of it, back when the layer was active).
    assembler.Run();
    const float bakedHeight = assembler.Fields().heightfield.Get(1, 1);

    layer.frequency = 0.9f;   // a parameter edit that WOULD move a live layer's noise
    assembler.Run();
    CheckLayerEditor(assembler.Fields().heightfield.Get(1, 1) == bakedHeight,
                     "a parameter edit after Bake does not move the frozen height, until Unbake");

    LayerEditorAction unbake;
    unbake.kind          = LayerEditorActionKind::BakeToggleRequested;
    unbake.geoLayerIndex  = 0;
    unbake.layerIndex     = 0;
    CheckLayerEditor(ApplyBakedImageAction(unbake, recipe.layerStack, assembler),
                     "Unbake reports the recipe moved");
    CheckLayerEditor(!layer.bBaked, "and flips the layer back to live");
    assembler.Run();
    CheckLayerEditor(assembler.Fields().heightfield.Get(1, 1) != liveHeight,
                     "and resumes live generation -- the edited frequency now DOES take effect");
}

// STEP151 acceptance (1): the real-world bug the human hit -- Bake/Unbake/Bake on an import must
// reproduce the exact original pixels bit-for-bit, never a fresh live-noise overwrite.
void RunBakeUnbakeRebakePreservesImportedPixelsChecks() {
    const std::string folderPath = ScratchFolderPath("SanGenLayerEditorRebakeTest");
    Params::MapRecipe recipe = FixtureRecipe();
    const std::string rawPath = WriteFixtureRawFile(folderPath, recipe.geometry.VertexSize());

    Pipeline::GenerationAssembler assembler(recipe);
    assembler.Thermal().Constants().iterationCount = 0;
    assembler.Run();
    Params::Layer& layer = recipe.layerStack.geoLayers[0].layers[0];

    LayerEditorAction import;
    import.kind          = LayerEditorActionKind::ImportRawRequested;
    import.geoLayerIndex  = 0;
    import.layerIndex     = 0;
    import.importRawPath  = rawPath;
    CheckLayerEditor(ApplyBakedImageAction(import, recipe.layerStack, assembler),
                     "STEP151: Import RAW succeeds");
    const int layerIdentifier = layer.layerIdentifier;
    const Data::FloatField importedSnapshot =
        *Data::FindBakedLayerImage(assembler.BakedLayerImages(), layerIdentifier);

    LayerEditorAction toggle;
    toggle.kind          = LayerEditorActionKind::BakeToggleRequested;
    toggle.geoLayerIndex  = 0;
    toggle.layerIndex     = 0;
    CheckLayerEditor(ApplyBakedImageAction(toggle, recipe.layerStack, assembler),
                     "STEP151: Unbake after import reports the recipe moved");
    CheckLayerEditor(!layer.bBaked, "STEP151: the layer resumes live generation");
    assembler.Run();   // NoiseBlend now caches PROCEDURAL noise, unrelated to the imported pixels

    CheckLayerEditor(ApplyBakedImageAction(toggle, recipe.layerStack, assembler),
                     "STEP151: re-baking after unbake reports the recipe moved");
    CheckLayerEditor(layer.bBaked, "STEP151: the layer is baked again");
    CheckLayerEditor(layer.layerIdentifier == layerIdentifier,
                     "STEP151: the SAME identifier is reused -- same cache entry, not a new one");

    const Data::FloatField* rebakedImage =
        Data::FindBakedLayerImage(assembler.BakedLayerImages(), layer.layerIdentifier);
    CheckLayerEditor(rebakedImage != nullptr, "STEP151: the rebaked entry still exists");
    bool bIdentical = rebakedImage->Width() == importedSnapshot.Width()
        && rebakedImage->Height() == importedSnapshot.Height();
    for (int y = 0; bIdentical && y < importedSnapshot.Height(); ++y)
        for (int x = 0; x < importedSnapshot.Width(); ++x)
            if (rebakedImage->Get(x, y) != importedSnapshot.Get(x, y)) { bIdentical = false; break; }
    CheckLayerEditor(bIdentical,
                     "STEP151: Bake/Unbake/Bake reproduces the original imported pixels bit-for-bit "
                     "(the old bug: the toggle unconditionally overwrote them with live noise)");
}

// STEP151 acceptance (3): Refresh Bake is the ONLY path that deliberately overwrites an existing
// snapshot -- the simple toggle must reuse it verbatim even after a live parameter edit.
void RunRefreshBakeChecks() {
    Params::MapRecipe recipe = FixtureRecipe();
    Pipeline::GenerationAssembler assembler(recipe);
    Params::Layer& layer = recipe.layerStack.geoLayers[0].layers[0];
    assembler.Run();

    LayerEditorAction toggle;
    toggle.kind          = LayerEditorActionKind::BakeToggleRequested;
    toggle.geoLayerIndex  = 0;
    toggle.layerIndex     = 0;
    CheckLayerEditor(ApplyBakedImageAction(toggle, recipe.layerStack, assembler),
                     "STEP151: first bake succeeds");
    const int layerIdentifier = layer.layerIdentifier;
    const float originalBakedHeight =
        Data::FindBakedLayerImage(assembler.BakedLayerImages(), layerIdentifier)->Get(1, 1);
    CheckLayerEditor(ApplyBakedImageAction(toggle, recipe.layerStack, assembler), "STEP151: unbake");

    layer.frequency = 0.9f;   // moves live noise -- NOT yet reflected in the existing snapshot
    assembler.Run();

    CheckLayerEditor(ApplyBakedImageAction(toggle, recipe.layerStack, assembler),
                     "STEP151: the simple toggle re-bakes");
    CheckLayerEditor(Data::FindBakedLayerImage(assembler.BakedLayerImages(), layerIdentifier)->Get(1, 1)
                    == originalBakedHeight,
                     "STEP151: ...but never overwrites the existing snapshot on its own");

    CheckLayerEditor(ApplyBakedImageAction(toggle, recipe.layerStack, assembler),
                     "STEP151: unbake again ahead of Refresh Bake");
    LayerEditorAction refresh;
    refresh.kind          = LayerEditorActionKind::RefreshBakeRequested;
    refresh.geoLayerIndex  = 0;
    refresh.layerIndex     = 0;
    CheckLayerEditor(ApplyBakedImageAction(refresh, recipe.layerStack, assembler),
                     "STEP151: Refresh Bake reports the recipe moved");
    CheckLayerEditor(!layer.bBaked, "STEP151: Refresh Bake does not itself flip bBaked");
    CheckLayerEditor(Data::FindBakedLayerImage(assembler.BakedLayerImages(), layerIdentifier)->Get(1, 1)
                    != originalBakedHeight,
                     "STEP151: ...but Refresh Bake DOES overwrite it with the current live noise");

    layer.bBaked = true;
    CheckLayerEditor(!ApplyBakedImageAction(refresh, recipe.layerStack, assembler),
                     "STEP151: Refresh Bake refuses an already-baked layer");
    layer.bBaked = false;
    layer.noiseType = Params::NoiseType::None;
    CheckLayerEditor(!ApplyBakedImageAction(refresh, recipe.layerStack, assembler),
                     "STEP151: and refuses a layer with no live recipe to refresh from");
}

// STEP151 acceptance (4): a stack edit that moves the layer's Params::Layer object in memory
// between NoiseBlend's last Run() and the click is refused, never mis-attributed -- constructed via
// a REAL identity-pointer mismatch (forced vector reallocation), not a guess.
void RunIdentityMismatchRefusalChecks() {
    Params::MapRecipe recipe = FixtureRecipe();
    recipe.layerStack.geoLayers[0].layers.shrink_to_fit();   // capacity == size == 1, exactly
    Pipeline::GenerationAssembler assembler(recipe);
    assembler.Run();   // caches CachedFlatLayerPointers() into the layer's CURRENT buffer address

    // Growing past the fixed capacity is GUARANTEED (the standard's capacity invariant) to move
    // every existing element to a brand-new heap buffer -- the layer's address changes even though
    // its C++ identity (the object a fresh index lookup resolves to) is unaffected, exactly the
    // "reordered/edited since NoiseBlend last ran" scenario the ticket calls out.
    recipe.layerStack.geoLayers[0].layers.push_back(Params::Layer());
    Params::Layer& layer = recipe.layerStack.geoLayers[0].layers[0];   // resolved fresh, like a real click

    LayerEditorAction bake;
    bake.kind          = LayerEditorActionKind::BakeToggleRequested;
    bake.geoLayerIndex  = 0;
    bake.layerIndex     = 0;
    CheckLayerEditor(!ApplyBakedImageAction(bake, recipe.layerStack, assembler),
                     "STEP151: a bake click after the layer moved in memory is refused");
    CheckLayerEditor(!layer.bBaked && layer.layerIdentifier == -1,
                     "STEP151: ...and the layer is left completely untouched by the refusal");

    LayerEditorAction refresh;
    refresh.kind          = LayerEditorActionKind::RefreshBakeRequested;
    refresh.geoLayerIndex  = 0;
    refresh.layerIndex     = 0;
    CheckLayerEditor(!ApplyBakedImageAction(refresh, recipe.layerStack, assembler),
                     "STEP151: Refresh Bake refuses the SAME identity mismatch rather than guessing");
}

void RunNamesLayerAndKindFenceChecks() {
    Params::MapRecipe recipe = FixtureRecipe();
    Pipeline::GenerationAssembler assembler(recipe);

    LayerEditorAction badIndex;
    badIndex.kind          = LayerEditorActionKind::BakeToggleRequested;
    badIndex.geoLayerIndex  = 4;
    badIndex.layerIndex     = 0;
    CheckLayerEditor(!ApplyBakedImageAction(badIndex, recipe.layerStack, assembler),
                     "an action naming no layer is refused");

    LayerEditorAction wrongKind;
    wrongKind.kind          = LayerEditorActionKind::AddLayer;
    wrongKind.geoLayerIndex  = 0;
    wrongKind.layerIndex     = 0;
    CheckLayerEditor(!ApplyBakedImageAction(wrongKind, recipe.layerStack, assembler),
                     "a structural kind is not this function's to apply");
}

} // namespace

void RunLayerEditorBakedImageChecks() {
    RunImportRawChecks();
    RunImportRawFailureChecks();
    RunBakeToggleChecks();
    RunBakeUnbakeRebakePreservesImportedPixelsChecks();
    RunRefreshBakeChecks();
    RunIdentityMismatchRefusalChecks();
    RunNamesLayerAndKindFenceChecks();
}

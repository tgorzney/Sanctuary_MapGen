// PreviewComposite_GpuProgram_UI.cpp — compiles the composite's GPU twin exactly once and
// builds the #define block that carries every C++ constant, enum value and binding index into
// the shader. Layer: UI. Nothing in the .glsl is hardcoded (Constitution §8) and the two sides
// cannot drift into separate numberings — the legacy preview kept a 15-slot program array in
// sync with its layer enum by hand, which is the defect this replaces.
// The kernel ships as two GLSL compilation units (one declares main(), the other provides the
// prototyped value math) so each file stays inside the ARCH §1.5 ceiling; GpuResource_SYS
// links them into a single compute program and resolves both names under its shader directory,
// never an absolute path.
#include "PreviewComposite_UI.h"
#include "../sys/GpuResource_SYS.h"
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {
namespace {

std::string IntegerDefinition(const char* name, int value) {
    return std::string("#define ") + name + " " + std::to_string(value) + "\n";
}

std::string BuildStageDefinitions() {
    return IntegerDefinition("PREVIEW_TILE_WIDTH",    Sys::WorkgroupSize::kFieldTileWidth)
         + IntegerDefinition("PREVIEW_TILE_HEIGHT",   Sys::WorkgroupSize::kFieldTileHeight)
         + IntegerDefinition("PREVIEW_STRATUM_COUNT", Data::MapFields::stratumCount)
         + IntegerDefinition("PREVIEW_LOOKUP_CHANNEL_COUNT", static_cast<int>(kLookupChannelCount))
         + IntegerDefinition("PREVIEW_PASS_CLEAR",             CompositePass::kClear)
         + IntegerDefinition("PREVIEW_PASS_FIELD_LAYER",       CompositePass::kFieldLayer)
         + IntegerDefinition("PREVIEW_PASS_OVERLAY",           CompositePass::kOverlay)
         + IntegerDefinition("PREVIEW_PASS_ENTITY_IDENTIFIER", CompositePass::kEntityIdentifier)
         + std::string("#define PREVIEW_EMPTY_ENTITY_SENTINEL ")
         + std::to_string(Data::EntityIdBuffer::emptySentinel) + "u\n";
}

std::string BuildEnumDefinitions() {
    return IntegerDefinition("PREVIEW_LAYER_HEIGHT_RAMP",   static_cast<int>(PreviewLayerKind::HeightRamp))
         + IntegerDefinition("PREVIEW_LAYER_STRATUM_SPLAT", static_cast<int>(PreviewLayerKind::StratumSplat))
         + IntegerDefinition("PREVIEW_LAYER_FLOW",          static_cast<int>(PreviewLayerKind::Flow))
         + IntegerDefinition("PREVIEW_LAYER_ACCUMULATION",  static_cast<int>(PreviewLayerKind::Accumulation))
         + IntegerDefinition("PREVIEW_LAYER_WATER",         static_cast<int>(PreviewLayerKind::Water))
         + IntegerDefinition("PREVIEW_LAYER_SLOPE",         static_cast<int>(PreviewLayerKind::Slope))
         + IntegerDefinition("PREVIEW_BLEND_REPLACE",       static_cast<int>(PreviewBlendMode::Replace))
         + IntegerDefinition("PREVIEW_BLEND_ALPHA",         static_cast<int>(PreviewBlendMode::AlphaBlend))
         + IntegerDefinition("PREVIEW_BLEND_ADD",           static_cast<int>(PreviewBlendMode::Add))
         + IntegerDefinition("PREVIEW_BLEND_MULTIPLY",      static_cast<int>(PreviewBlendMode::Multiply))
         + IntegerDefinition("PREVIEW_BLEND_MAXIMUM",       static_cast<int>(PreviewBlendMode::Maximum))
         + IntegerDefinition("PREVIEW_BLEND_MINIMUM",       static_cast<int>(PreviewBlendMode::Minimum));
}

std::string BuildBindingDefinitions() {
    return IntegerDefinition("PREVIEW_BINDING_ENTITY_IDENTIFIERS", static_cast<int>(CompositeBinding::kEntityIdentifiers))
         + IntegerDefinition("PREVIEW_BINDING_HEIGHTFIELD",        static_cast<int>(CompositeBinding::kHeightfield))
         + IntegerDefinition("PREVIEW_BINDING_FLOW",               static_cast<int>(CompositeBinding::kFlow))
         + IntegerDefinition("PREVIEW_BINDING_ACCUMULATION",       static_cast<int>(CompositeBinding::kAccumulation))
         + IntegerDefinition("PREVIEW_BINDING_SLOPE",              static_cast<int>(CompositeBinding::kSlope))
         + IntegerDefinition("PREVIEW_BINDING_SURFACE_WEIGHTS",    static_cast<int>(CompositeBinding::kSurfaceStratumWeights))
         + IntegerDefinition("PREVIEW_BINDING_GRADIENT_TABLES",    static_cast<int>(CompositeBinding::kGradientLookupTables))
         + IntegerDefinition("PREVIEW_BINDING_ENTITY_POINTS",      static_cast<int>(CompositeBinding::kEntityPoints))
         + IntegerDefinition("PREVIEW_BINDING_COMPOSITE_TEXELS",   static_cast<int>(CompositeBinding::kCompositeTexels))
         + IntegerDefinition("PREVIEW_BINDING_CONFIGURATION",      static_cast<int>(CompositeBinding::kConfiguration))
         + IntegerDefinition("PREVIEW_BINDING_LAYERS",             static_cast<int>(CompositeBinding::kLayerConfigurations))
         + IntegerDefinition("PREVIEW_BINDING_STRATA",             static_cast<int>(CompositeBinding::kStratumConfigurations));
}

// main() first, then the prototyped providers: the field sampling/colorization unit and the
// pure value math. Names only — GpuResource_SYS resolves them under its configured shader
// directory (PREVIEW_COMPOSITING_SPEC: the legacy renderer's absolute shader path produced a
// blank preview whenever it missed).
const std::vector<std::string>& ProgramParts() {
    static const std::vector<std::string> parts = { "PreviewComposite_UI.glsl",
                                                    "PreviewComposite_Sampling_UI.glsl",
                                                    "PreviewComposite_Color_UI.glsl" };
    return parts;
}

} // namespace

// The manager's compile-once cache is keyed by (files + definitions), so an unchanged set costs
// a string compare, never a recompile.
bool PreviewComposite::EnsureGpuResources() {
    if (gpuResourceManager == nullptr) return false;
    if (!gpuResourceManager->IsInitialized() && !gpuResourceManager->Initialize()) return false;

    const std::string definitions =
        BuildStageDefinitions() + BuildEnumDefinitions() + BuildBindingDefinitions();
    const Sys::GpuProgramHandle program =
        gpuResourceManager->GetOrCompileProgramFromParts(ProgramParts(), definitions);
    if (!program.IsValid()) return false;

    gpuProgramIndex = program.programIndex;
    bGpuProgramReady = true;
    return true;
}

} // namespace Ui
} // namespace SanmapGen

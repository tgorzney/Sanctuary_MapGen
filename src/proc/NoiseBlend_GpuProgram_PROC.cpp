// NoiseBlend_GpuProgram_PROC.cpp — compiles the stage's GPU twin exactly once and builds the
// #define block that carries every C++ constant and enum value into the shader, so nothing in
// the .glsl is hardcoded (Constitution §8) and the two sides cannot drift (the enum values are
// read from the enums themselves). The kernel ships as several GLSL compilation units — one
// declares main(), the rest are prototyped function providers — so each file stays inside the
// ARCH §1.5 ceiling; GpuResource_SYS links them into a single compute program.
#include "NoiseBlend_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include <cstdio>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Proc {
namespace {

std::string IntegerDefinition(const char* name, int value) {
    return std::string("#define ") + name + " " + std::to_string(value) + "\n";
}

// A float has to reach GLSL as a float literal, so a whole number still needs its ".0".
std::string FloatDefinition(const char* name, float value) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.9g", static_cast<double>(value));
    std::string literal(text);
    if (literal.find('.') == std::string::npos && literal.find('e') == std::string::npos)
        literal += ".0";
    return std::string("#define ") + name + " " + literal + "\n";
}

std::string BuildEnumDefinitions() {
    return IntegerDefinition("NOISE_TYPE_OPEN_SIMPLEX2",        static_cast<int>(Params::NoiseType::OpenSimplex2))
         + IntegerDefinition("NOISE_TYPE_OPEN_SIMPLEX2_SMOOTH", static_cast<int>(Params::NoiseType::OpenSimplex2Smooth))
         + IntegerDefinition("NOISE_TYPE_CELLULAR",             static_cast<int>(Params::NoiseType::Cellular))
         + IntegerDefinition("NOISE_TYPE_PERLIN",               static_cast<int>(Params::NoiseType::Perlin))
         + IntegerDefinition("NOISE_TYPE_VALUE_CUBIC",          static_cast<int>(Params::NoiseType::ValueCubic))
         + IntegerDefinition("NOISE_TYPE_VALUE",                static_cast<int>(Params::NoiseType::Value))
         + IntegerDefinition("NOISE_TYPE_NONE",                 static_cast<int>(Params::NoiseType::None))
         + IntegerDefinition("FRACTAL_TYPE_NONE",               static_cast<int>(Params::FractalType::None))
         + IntegerDefinition("FRACTAL_TYPE_FRACTIONAL_BROWNIAN",static_cast<int>(Params::FractalType::FractionalBrownian))
         + IntegerDefinition("FRACTAL_TYPE_RIDGED",             static_cast<int>(Params::FractalType::Ridged))
         + IntegerDefinition("FRACTAL_TYPE_PING_PONG",          static_cast<int>(Params::FractalType::PingPong))
         + IntegerDefinition("HEIGHT_BLEND_ADD",                static_cast<int>(Params::HeightBlendMode::Add))
         + IntegerDefinition("HEIGHT_BLEND_SUBTRACT",           static_cast<int>(Params::HeightBlendMode::Subtract))
         + IntegerDefinition("HEIGHT_BLEND_MULTIPLY",           static_cast<int>(Params::HeightBlendMode::Multiply))
         + IntegerDefinition("HEIGHT_BLEND_OVERLAY",            static_cast<int>(Params::HeightBlendMode::Overlay))
         + IntegerDefinition("HEIGHT_BLEND_MAXIMUM",            static_cast<int>(Params::HeightBlendMode::Maximum))
         + IntegerDefinition("HEIGHT_BLEND_MINIMUM",            static_cast<int>(Params::HeightBlendMode::Minimum));
}

std::string BuildStageDefinitions(const NoiseBlendConstants& constants) {
    return IntegerDefinition("NOISE_BLEND_TILE_WIDTH",  Sys::WorkgroupSize::kFieldTileWidth)
         + IntegerDefinition("NOISE_BLEND_TILE_HEIGHT", Sys::WorkgroupSize::kFieldTileHeight)
         + IntegerDefinition("NOISE_BLEND_STRATUM_COUNT", Data::MapFields::stratumCount)
         + IntegerDefinition("NOISE_BLEND_PASS_NOISE", NoiseBlendPassMode::Noise)
         + IntegerDefinition("NOISE_BLEND_PASS_BLEND", NoiseBlendPassMode::Blend)
         + IntegerDefinition("NOISE_BLEND_BINDING_LAYERS", static_cast<int>(NoiseBlendBinding::layerConfigurations))
         + IntegerDefinition("NOISE_BLEND_BINDING_RAW_NOISE", static_cast<int>(NoiseBlendBinding::rawNoise))
         + IntegerDefinition("NOISE_BLEND_BINDING_HEIGHT", static_cast<int>(NoiseBlendBinding::heightField))
         + IntegerDefinition("NOISE_BLEND_BINDING_MASKS", static_cast<int>(NoiseBlendBinding::materialMasks))
         + IntegerDefinition("NOISE_BLEND_BINDING_THICKNESS", static_cast<int>(NoiseBlendBinding::layerThickness))
         + FloatDefinition("NOISE_BLEND_RAW_OFFSET", constants.rawNoiseOffset)
         + FloatDefinition("NOISE_BLEND_RAW_SCALE",  constants.rawNoiseScale);
}

// main() first, then the prototyped providers; order is irrelevant to the linker but keeps
// the kernel readable. Names only — GpuResource_SYS resolves them under its shader directory,
// never an absolute path (NOISE_BLEND_SPEC "hardcoded absolute shader paths").
const std::vector<std::string>& ProgramParts() {
    static const std::vector<std::string> parts = {
        "NoiseBlend_PROC.glsl",   "NoiseBlend_Shape_PROC.glsl",  "NoiseBlend_Fractal_PROC.glsl",
        "NoiseBlend_Hash_PROC.glsl", "NoiseBlend_Simplex_PROC.glsl",
        "NoiseBlend_SimplexSmooth_PROC.glsl", "NoiseBlend_Lattice_PROC.glsl",
        "NoiseBlend_Cellular_PROC.glsl" };
    return parts;
}

} // namespace

// Called every run rather than once: the definitions embed the tweakable stage constants, so
// changing one must yield a different program. The manager's compile-once cache is keyed by
// (files + definitions), so an unchanged set costs a string compare, never a recompile.
bool NoiseBlendStage::EnsureGpuResources() {
    if (gpuResourceManager == nullptr) return false;
    if (!gpuResourceManager->IsInitialized() && !gpuResourceManager->Initialize()) return false;

    const std::string definitions = BuildStageDefinitions(constants) + BuildEnumDefinitions();
    const Sys::GpuProgramHandle program =
        gpuResourceManager->GetOrCompileProgramFromParts(ProgramParts(), definitions);
    if (!program.IsValid()) return false;

    gpuProgramIndex = program.programIndex;
    bGpuProgramReady = true;
    return true;
}

} // namespace Proc
} // namespace SanmapGen

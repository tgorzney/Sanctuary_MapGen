// LayerEditor_Scalars_UI.cpp — the Layer Editor's limit table and the state that copies it.
// Layer: UI. No imgui here on purpose: this is the one Layer Editor translation unit the headless
// acceptance test pulls in, so constructing a LayerEditorState needs no window and no GL context.
// The rows are exactly TAB_REBUILD_PLAN "§ Layer Editor"; every limit is a named setting, never a
// literal at a use site (Constitution §8).
#include "LayerEditor_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// { label, { minimum, maximum, increment }, printf format, is-integer }
const LayerEditorScalarDescription scalarDescriptions[kLayerEditorScalarCount] = {
    { "Group Stratum Index",     { 0.0f,     8.0f,       1.0f     }, "%d",     true  },
    { "Stratum Index",           { 0.0f,     8.0f,       1.0f     }, "%d",     true  },
    { "Opacity",                 { 0.0f,     1.0f,       0.0f     }, "%.3f",   false },
    { "Image Contrast",          { 0.0f,     3.0f,       0.0f     }, "%.3f",   false },
    { "Frequency",               { 0.0001f,  0.5f,       0.0f     }, "%.4f",   false },
    { "Octaves",                 { 1.0f,     10.0f,      1.0f     }, "%d",     true  },
    { "Gain",                    { 0.1f,     5.0f,       0.0f     }, "%.3f",   false },
    { "Lacunarity",              { 1.0f,     4.0f,       0.0f     }, "%.3f",   false },
    { "Weighted Strength",       { 0.0f,     1.0f,       0.0f     }, "%.3f",   false },
    { "Ping-Pong Strength",      { 0.0f,     5.0f,       0.0f     }, "%.3f",   false },
    { "Cellular Jitter",         { 0.0f,     1.0f,       0.0f     }, "%.3f",   false },
    { "Land Density",            { 0.0f,     1.0f,       0.0f     }, "%.3f",   false },
    { "Plateau Density",         { 0.0f,     1.0f,       0.0f     }, "%.3f",   false },
    { "Mountain Density",        { 0.0f,     1.0f,       0.0f     }, "%.3f",   false },
    { "Ramp Density",            { 0.0f,     1.0f,       0.0f     }, "%.3f",   false },
    { "Hardness",                { 0.01f,    1.0f,       0.0f     }, "%.3f",   false },
    { "Friction",                { 0.01f,    1.0f,       0.0f     }, "%.3f",   false },
    { "Cohesion",                { 0.01f,    1.0f,       0.0f     }, "%.3f",   false },
    { "Capacity Multiplier",     { 0.1f,     5.0f,       0.0f     }, "%.3f",   false },
    { "Absorption Rate",         { 0.001f,   0.5f,       0.0f     }, "%.4f",   false },
    { "Droplet Count",           { 1000.0f,  5000000.0f, 1000.0f  }, "%d",     true  },
    { "Max Lifetime",            { 5.0f,     200.0f,     1.0f     }, "%d",     true  },
    { "Evaporation",             { 0.001f,   0.2f,       0.0f     }, "%.4f",   false },
    { "Viscosity",               { 0.1f,     10.0f,      0.0f     }, "%.3f",   false },
    { "Capacity Scale",          { 0.1f,     10.0f,      0.0f     }, "%.3f",   false },
    { "Gravity",                 { 0.5f,     20.0f,      0.0f     }, "%.3f",   false },
    { "Rain Noise Frequency",    { 0.001f,   0.1f,       0.0f     }, "%.4f",   false },
    { "Rain Noise Octaves",      { 1.0f,     8.0f,       1.0f     }, "%d",     true  },
    { "Wind Angle",              { 0.0f,     360.0f,     0.0f     }, "%.1f",   false },
    { "Initial Load",            { 0.01f,    5.0f,       0.0f     }, "%.3f",   false },
    { "Base Erosion Rate",       { 0.0f,     1.0f,       0.0f     }, "%.4f",   false },
    { "Base Deposition Rate",    { 0.0f,     1.0f,       0.0f     }, "%.4f",   false },
    { "Meander Strength",        { 0.0f,     1.0f,       0.0f     }, "%.4f",   false },
    { "Divergence Threshold",    { 0.0f,     5.0f,       0.0f     }, "%.4f",   false },
    { "Thermal Iterations",      { 1.0f,     64.0f,      1.0f     }, "%d",     true  },
    { "Thermal Rate",            { 0.0f,     1.0f,       0.0f     }, "%.4f",   false },
};

const LayerEditorScalarDescription emptyScalarDescription;

} // namespace

const LayerEditorScalarDescription& LayerEditorScalarDescriptionOf(LayerEditorScalar scalar) {
    const int scalarIndex = static_cast<int>(scalar);
    if (scalarIndex < 0 || scalarIndex >= kLayerEditorScalarCount) return emptyScalarDescription;
    return scalarDescriptions[scalarIndex];
}

LayerEditorState::LayerEditorState() {
    for (int scalarIndex = 0; scalarIndex < kLayerEditorScalarCount; ++scalarIndex)
        scalarRanges[scalarIndex] = scalarDescriptions[scalarIndex].range;
    SectionOptions closedByDefault;
    closedByDefault.bDefaultOpen = false;
    advancedConstantsSection = InitialSectionState(closedByDefault);
    importRawOptions.allowedExtensions = ".raw;.r16";
}

} // namespace Ui
} // namespace SanmapGen

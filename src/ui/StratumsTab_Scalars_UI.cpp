// StratumsTab_Scalars_UI.cpp — the Stratums tab's limit table and the state that copies it.
// Layer: UI. No imgui here on purpose: this is the one Stratums translation unit the headless
// acceptance test pulls in, so constructing a StratumsTabState needs no window and no GL context.
// The rows are exactly TAB_REBUILD_PLAN "6 · Stratums"; every limit is a named setting, never a
// literal at a use site (Constitution §8).
#include "StratumsTab_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// { label, { minimum, maximum, increment }, printf format }
const StratumsTabScalarDescription scalarDescriptions[kStratumsTabScalarCount] = {
    { "Mask Remap Min",       {  0.0f,   10.0f,   0.0f }, "%.3f" },
    { "Mask Remap Max",       {  0.0f,   10.0f,   0.0f }, "%.3f" },
    { "Tile Size Far",        {  0.1f, 1000.0f,   0.0f }, "%.2f" },
    { "Triplanar Tile",       {  0.1f,  100.0f,   0.0f }, "%.2f" },
    { "Far Triplanar Tile",   {  0.1f,  100.0f,   0.0f }, "%.2f" },
    { "Tile Size",            {  0.1f, 1000.0f,   0.0f }, "%.2f" },
    { "Normal Scale",         {  0.0f,    5.0f,   0.0f }, "%.3f" },
    { "Normal Scale Far",     {  0.0f,    5.0f,   0.0f }, "%.3f" },
    { "Normal Far/Near Blend",{  0.0f,    1.0f,   0.0f }, "%.3f" },
    { "Height Far/Near Blend",{  0.0f,    1.0f,   0.0f }, "%.3f" },
    { "Hardness",             {  0.01f,   1.0f,   0.0f }, "%.3f" },
    { "Friction",             {  0.01f,   1.0f,   0.0f }, "%.3f" },
    { "Cohesion",             {  0.01f,   1.0f,   0.0f }, "%.3f" },
    { "Capacity Multiplier",  {  0.1f,    5.0f,   0.0f }, "%.3f" },
    { "Absorption Rate",      {  0.001f,  0.5f,   0.0f }, "%.4f" },
};

const StratumsTabScalarDescription emptyScalarDescription;

} // namespace

const StratumsTabScalarDescription& StratumsTabScalarDescriptionOf(StratumsTabScalar scalar) {
    const int scalarIndex = static_cast<int>(scalar);
    if (scalarIndex < 0 || scalarIndex >= kStratumsTabScalarCount) return emptyScalarDescription;
    return scalarDescriptions[scalarIndex];
}

StratumsTabState::StratumsTabState() {
    for (int scalarIndex = 0; scalarIndex < kStratumsTabScalarCount; ++scalarIndex)
        scalarRanges[scalarIndex] = scalarDescriptions[scalarIndex].range;
    environmentPackOptions.allowedExtensions = ".sanpack;.zip";
    environmentPackOptions.browseButtonLabel = "Select Environment .sanpack";
    textureOptions.allowedExtensions         = ".dds;.png;.tga;.jpg";
    // Nine sections open at once is a wall of sliders, so a stratum opens only when asked — the
    // same shape v1's collapsing headers had.
    SectionOptions closedByDefault;
    closedByDefault.bDefaultOpen = false;
    for (int stratumIndex = 0; stratumIndex < kStratumsTabStratumCount; ++stratumIndex)
        rows[stratumIndex].section = InitialSectionState(closedByDefault);
}

} // namespace Ui
} // namespace SanmapGen

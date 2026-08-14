#version 430 core
// Thermal_PROC.glsl — Gpu speed path of the talus-relaxation stage; twin of Thermal_PROC.cpp.
// Two passes over the same program, mirroring the Cpu sweep exactly: THERMAL_PASS_PREPARE
// publishes each cell's talus threshold (blended from its material masks) and its spread
// factor; THERMAL_PASS_APPLY gathers — every cell subtracts what it owes and adds what each
// neighbour owes it, reading heightRead and writing heightWrite (ping-pong, so no invocation
// ever writes a cell another invocation reads this pass). That retires the legacy
// AvalancheCompute non-atomic scatter AND its hardcoded "/2.0": every float here arrives in
// the kernelConstants block built by the Cpu (Constitution §8).
layout(local_size_x = THERMAL_TILE_WIDTH, local_size_y = THERMAL_TILE_HEIGHT) in;

layout(std430, binding = 0) readonly  buffer HeightRead      { float heightRead[]; };
layout(std430, binding = 1) writeonly buffer HeightWrite     { float heightWrite[]; };
layout(std430, binding = 2) readonly  buffer MaskRead        { float maskRead[]; };
layout(std430, binding = 3) writeonly buffer MaskWrite       { float maskWrite[]; };
layout(std430, binding = 4)           buffer SpreadFactor    { float cellSpreadFactor[]; };
layout(std430, binding = 5)           buffer TalusThreshold  { float cellTalusThreshold[]; };
// Sized, not unsized: the talus slots are read at THERMAL_SLOT_TALUS_BASE + stratum, and a
// driver can only prove that constant-folded index in range against a declared length.
layout(std430, binding = 6) readonly  buffer KernelConstants { float kernelConstants[THERMAL_CONSTANT_COUNT]; };

uniform int vertexSize;
uniform int passMode;
uniform int transportMaterialMasks;

// Visit order is load-bearing: it must match thermalNeighbourOffsetX/Y in Thermal_Kernel_PROC.h
// so both backends accumulate the same floats in the same order. Spelled as a function, not a
// const array, so no driver has to prove a local array index is in range.
ivec2 thermalNeighbourOffset(int step) {
    if (step == 0) return ivec2(-1,  0);
    if (step == 1) return ivec2( 1,  0);
    if (step == 2) return ivec2( 0, -1);
    return ivec2(0, 1);
}

// Mirrors Proc::ExcessDrop.
float excessDrop(float higherHeight, float lowerHeight, float talusThreshold) {
    float excess = higherHeight - lowerHeight - talusThreshold;
    return excess > 0.0 ? excess : 0.0;
}

bool insideField(ivec2 cell) {
    return cell.x >= 0 && cell.x < vertexSize && cell.y >= 0 && cell.y < vertexSize;
}

// Mirrors BlendCellTalusThreshold in Thermal_Relax_PROC.cpp.
float blendCellTalusThreshold(int cellIndex, int cellCount) {
    float weightSum = 0.0;
    float weightedThreshold = 0.0;
    for (int stratum = 0; stratum < THERMAL_STRATUM_COUNT; ++stratum) {
        float weight = maskRead[stratum * cellCount + cellIndex];
        weightSum += weight;
        weightedThreshold += weight * kernelConstants[THERMAL_SLOT_TALUS_BASE + stratum];
    }
    if (weightSum > kernelConstants[THERMAL_SLOT_MASK_EPSILON])
        return weightedThreshold * (1.0 / weightSum);
    return kernelConstants[THERMAL_SLOT_TALUS_BASE];
}

void runPreparePass(ivec2 cell, int cellIndex, int cellCount) {
    float threshold = blendCellTalusThreshold(cellIndex, cellCount);
    cellTalusThreshold[cellIndex] = threshold;
    float height = heightRead[cellIndex];
    float totalExcess = 0.0;
    for (int step = 0; step < THERMAL_NEIGHBOUR_COUNT; ++step) {
        ivec2 neighbour = cell + thermalNeighbourOffset(step);
        if (!insideField(neighbour)) continue;
        totalExcess += excessDrop(height, heightRead[neighbour.y * vertexSize + neighbour.x], threshold);
    }
    cellSpreadFactor[cellIndex] = totalExcess > kernelConstants[THERMAL_SLOT_MOVEMENT_EPSILON]
                                ? kernelConstants[THERMAL_SLOT_SPREAD_FACTOR] : 0.0;
}

// The gather. Donor material is summed per stratum inside the SAME neighbour loop (indexed only
// by loop counters — no dynamic local-array writes, which no GLSL driver has to support), in the
// same order MixCellMaterial uses on the Cpu.
void runApplyPass(ivec2 cell, int cellIndex, int cellCount) {
    float height = heightRead[cellIndex];
    float threshold = cellTalusThreshold[cellIndex];
    float spreadFactor = cellSpreadFactor[cellIndex];
    float donorMask[THERMAL_STRATUM_COUNT];
    for (int stratum = 0; stratum < THERMAL_STRATUM_COUNT; ++stratum) donorMask[stratum] = 0.0;
    float outflow = 0.0;
    float totalInflow = 0.0;
    for (int step = 0; step < THERMAL_NEIGHBOUR_COUNT; ++step) {
        ivec2 neighbour = cell + thermalNeighbourOffset(step);
        if (!insideField(neighbour)) continue;
        int index = neighbour.y * vertexSize + neighbour.x;
        float neighbourHeight = heightRead[index];
        outflow += spreadFactor * excessDrop(height, neighbourHeight, threshold);
        float received = cellSpreadFactor[index]
                       * excessDrop(neighbourHeight, height, cellTalusThreshold[index]);
        totalInflow += received;
        if (transportMaterialMasks != 0)
            for (int stratum = 0; stratum < THERMAL_STRATUM_COUNT; ++stratum)
                donorMask[stratum] += received * maskRead[stratum * cellCount + index];
    }
    heightWrite[cellIndex] = height - outflow + totalInflow;

    if (transportMaterialMasks == 0) return;
    if (totalInflow <= kernelConstants[THERMAL_SLOT_MOVEMENT_EPSILON]) {
        for (int stratum = 0; stratum < THERMAL_STRATUM_COUNT; ++stratum)
            maskWrite[stratum * cellCount + cellIndex] = maskRead[stratum * cellCount + cellIndex];
        return;
    }
    float minimumColumnDepth = kernelConstants[THERMAL_SLOT_MINIMUM_COLUMN_DEPTH];
    float columnDepth = height > minimumColumnDepth ? height : minimumColumnDepth;
    float inverseTotal = 1.0 / (columnDepth + totalInflow);
    for (int stratum = 0; stratum < THERMAL_STRATUM_COUNT; ++stratum)
        maskWrite[stratum * cellCount + cellIndex] =
            (maskRead[stratum * cellCount + cellIndex] * columnDepth + donorMask[stratum]) * inverseTotal;
}

void main() {
    ivec2 cell = ivec2(gl_GlobalInvocationID.xy);
    if (cell.x >= vertexSize || cell.y >= vertexSize) return;
    int cellIndex = cell.y * vertexSize + cell.x;
    int cellCount = vertexSize * vertexSize;
    if (passMode == THERMAL_PASS_PREPARE) runPreparePass(cell, cellIndex, cellCount);
    else                                  runApplyPass(cell, cellIndex, cellCount);
}

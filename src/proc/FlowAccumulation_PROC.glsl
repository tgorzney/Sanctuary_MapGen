#version 430 core
// FlowAccumulation_PROC.glsl — the GPU twin of the FlowAccumulation_PROC.cpp family.
// One file, three programs, selected by the #define the SYS resource manager injects
// (FLOW_PASS_FILL / FLOW_PASS_DIRECTION / FLOW_PASS_ACCUMULATION) so all three share one
// copy of the neighbour table and the noise hash — no third copy of anything.
//   FILL         Planchon-Darboux relaxation: surface = max(height, min(neighbours) + eps).
//                Its fixed point IS the CPU priority-flood answer, so this converges to the
//                same surface rather than approximating it.
//   DIRECTION    the same stochastic single-flow-direction rule as RouteCell(), same order.
//   ACCUMULATION race-free GATHER (a cell pulls from the neighbours whose direction points
//                back at it via opposite(k) == k ^ 1) — never a float scatter, so the old
//                "no atomics for floats" race cannot exist here.
// Ping-pong buffers are rebound per iteration by the host; only int convergence counters use
// atomics. Every constant arrives from the C++ side (Constitution §8) — nothing hardcoded.
layout(local_size_x = WORKGROUP_TILE_WIDTH, local_size_y = WORKGROUP_TILE_HEIGHT) in;

layout(std430, binding = 0) readonly  buffer HeightBlock              { float heightValues[]; };
layout(std430, binding = 1) readonly  buffer SurfaceSourceBlock       { float surfaceSource[]; };
layout(std430, binding = 2) writeonly buffer SurfaceTargetBlock       { float surfaceTarget[]; };
layout(std430, binding = 3)           buffer DirectionBlock           { int flowDirection[]; };
layout(std430, binding = 4) writeonly buffer MagnitudeBlock           { float flowMagnitude[]; };
layout(std430, binding = 5)           buffer AccumulationSourceBlock  { float accumulationSource[]; };
layout(std430, binding = 6) writeonly buffer AccumulationTargetBlock  { float accumulationTarget[]; };
// Mirrors FlowAccumulationKernelRecord field for field (DISPATCH_INTERFACE_SPEC §4).
layout(std430, binding = 7) readonly buffer ConstantBlock {
    float cellWeight;
    float flowNoiseImpact;
    float depressionFillEpsilon;
    float cardinalInverseDistance;
    float diagonalInverseDistance;
    float flowMagnitudeScale;
    float unusedPaddingA;
    float unusedPaddingB;
};
layout(std430, binding = 8) buffer ChangedBlock { int changedCellCount; };

uniform int vertexSize;
uniform int flowNoiseSeed;

// Neighbour order mirrors flowNeighbourOffsetX/Y; opposite(index) == (index ^ 1).
const ivec2 flowNeighbourOffset[8] = ivec2[8](
    ivec2(-1,  0), ivec2( 1,  0), ivec2( 0, -1), ivec2( 0,  1),
    ivec2(-1, -1), ivec2( 1,  1), ivec2( 1, -1), ivec2(-1,  1));
const bool bFlowNeighbourIsDiagonal[8] = bool[8](false, false, false, false, true, true, true, true);

bool insideGrid(ivec2 cell) {
    return cell.x >= 0 && cell.y >= 0 && cell.x < vertexSize && cell.y < vertexSize;
}
int cellIndexOf(ivec2 cell) { return cell.y * vertexSize + cell.x; }

// Integer-only mixing, identical to FlowNoiseHash / FlowNoiseUnit in the kernel header.
uint flowNoiseHash(uint cellX, uint cellY, uint neighbourIndex, uint seed) {
    uint mixed = cellX * 374761393u + cellY * 668265263u
               + neighbourIndex * 2246822519u + seed * 3266489917u;
    mixed = (mixed ^ (mixed >> 13u)) * 1274126177u;
    return mixed ^ (mixed >> 16u);
}
float flowNoiseUnit(uint cellX, uint cellY, uint neighbourIndex, uint seed) {
    return float(flowNoiseHash(cellX, cellY, neighbourIndex, seed) >> 8u) * (1.0 / 16777216.0);
}

#ifdef FLOW_PASS_FILL
void runPass(ivec2 cell, int index) {
    float terrainHeight = heightValues[index];
    if (cell.x == 0 || cell.y == 0 || cell.x == vertexSize - 1 || cell.y == vertexSize - 1) {
        surfaceTarget[index] = terrainHeight;   // the border drains off the map
        return;
    }
    float lowestNeighbour = FLOW_UNFILLED_SURFACE_HEIGHT;
    for (int neighbour = 0; neighbour < 8; ++neighbour) {
        ivec2 neighbourCell = cell + flowNeighbourOffset[neighbour];
        if (!insideGrid(neighbourCell)) continue;
        lowestNeighbour = min(lowestNeighbour, surfaceSource[cellIndexOf(neighbourCell)]);
    }
    float raisedHeight = max(terrainHeight, lowestNeighbour + depressionFillEpsilon);
    surfaceTarget[index] = raisedHeight;
    if (raisedHeight != surfaceSource[index]) atomicAdd(changedCellCount, 1);
}
#endif

#ifdef FLOW_PASS_DIRECTION
void runPass(ivec2 cell, int index) {
    float cellHeight = surfaceSource[index];
    int bestDirection = -1;
    float bestScore = 0.0;
    float bestSlope = 0.0;
    for (int neighbour = 0; neighbour < 8; ++neighbour) {
        ivec2 neighbourCell = cell + flowNeighbourOffset[neighbour];
        if (!insideGrid(neighbourCell)) continue;
        float drop = cellHeight - surfaceSource[cellIndexOf(neighbourCell)];
        if (drop <= 0.0) continue;
        float inverseDistance = bFlowNeighbourIsDiagonal[neighbour] ? diagonalInverseDistance
                                                                   : cardinalInverseDistance;
        float slope = drop * inverseDistance;
        float score = slope + flowNoiseUnit(uint(cell.x), uint(cell.y), uint(neighbour),
                                            uint(flowNoiseSeed)) * flowNoiseImpact;
        if (score > bestScore) {
            bestScore = score;
            bestDirection = neighbour;
            bestSlope = slope;
        }
    }
    flowDirection[index] = bestDirection;
    flowMagnitude[index] = bestSlope * flowMagnitudeScale;
    accumulationSource[index] = cellWeight;   // seeds the relaxation below
}
#endif

#ifdef FLOW_PASS_ACCUMULATION
void runPass(ivec2 cell, int index) {
    float total = cellWeight;
    for (int neighbour = 0; neighbour < 8; ++neighbour) {
        ivec2 neighbourCell = cell + flowNeighbourOffset[neighbour];
        if (!insideGrid(neighbourCell)) continue;
        int neighbourIndex = cellIndexOf(neighbourCell);
        if (flowDirection[neighbourIndex] == (neighbour ^ 1)) total += accumulationSource[neighbourIndex];
    }
    accumulationTarget[index] = total;
    if (total != accumulationSource[index]) atomicAdd(changedCellCount, 1);
}
#endif

// Exactly one runPass() survives the preprocessor, so each program is a single pass.
void main() {
    ivec2 cell = ivec2(gl_GlobalInvocationID.xy);
    if (!insideGrid(cell)) return;
    runPass(cell, cellIndexOf(cell));
}

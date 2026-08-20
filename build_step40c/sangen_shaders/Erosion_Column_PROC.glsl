#version 430 core
// Erosion_Column_PROC.glsl — Gpu twin of Erosion_Column_PROC.h: the per-column operations.
// This unit owns the erosion state and the material buffer; Erosion_PROC.glsl calls in by
// prototype. THE fix this stage exists for: the state is fixed-point INTEGER ticks, so
// deposition is an atomicAdd and carving is an atomicCompSwap loop clamped to what the
// column actually holds. Integer atomics are exact and order-independent — the old
// "thicknesses[i] += amount" float read-modify-write race is gone, and no droplet can take
// material that is not there (SIM_ALGORITHMS_SPEC "Rework — GPU correctness").

layout(std430, binding = 1) buffer ThicknessState { int thicknessFixedPoint[]; };
layout(std430, binding = 3) readonly buffer MaterialPhysicsBuffer { float materialPhysics[]; };

// The shared random stream — mirrors HashRandom* in Erosion_Kernel_PROC.h bit for bit, so a
// droplet meanders the same way on both backends (uint wraps identically in C++ and GLSL).
uint hashRandomUnsigned(uint state) {
    state ^= state >> 16; state *= 0x7feb352du;
    state ^= state >> 15; state *= 0x846ca68bu;
    state ^= state >> 16; return state;
}
uint hashRandomCombine(uint first, uint second) { return hashRandomUnsigned(first * 0x9e3779b9u + second); }
float hashRandomUnitFloat(uint state) { return float(hashRandomUnsigned(state) >> 8) * (1.0 / 16777216.0); }

int heightToFixedPoint(float height, float fixedPointScale) {
    float scaled = height * fixedPointScale;
    return int(scaled + (scaled >= 0.0 ? 0.5 : -0.5));
}

float fixedPointToHeight(int fixedTicks, float fixedPointInverse) {
    return float(fixedTicks) * fixedPointInverse;
}

int columnTotalFixedPoint(int stratumCount, int cellCount, int cellIndex) {
    int total = 0;
    for (int stratum = 0; stratum < stratumCount; ++stratum)
        total += thicknessFixedPoint[stratum * cellCount + cellIndex];
    return total;
}

// Returns (height, gradientX, gradientY) in height units — same clamped bilinear as the Cpu.
vec3 sampleColumnHeight(int stratumCount, int vertexSize, float fixedPointInverse, float sampleX, float sampleY) {
    int cellCount = vertexSize * vertexSize;
    int coordinateX = int(sampleX);
    int coordinateY = int(sampleY);
    float fractionX = sampleX - float(coordinateX);
    float fractionY = sampleY - float(coordinateY);
    int lowX  = clamp(coordinateX,     0, vertexSize - 1);
    int lowY  = clamp(coordinateY,     0, vertexSize - 1);
    int highX = clamp(coordinateX + 1, 0, vertexSize - 1);
    int highY = clamp(coordinateY + 1, 0, vertexSize - 1);

    float heightLowLow   = fixedPointToHeight(columnTotalFixedPoint(stratumCount, cellCount, lowY  * vertexSize + lowX),  fixedPointInverse);
    float heightHighLow  = fixedPointToHeight(columnTotalFixedPoint(stratumCount, cellCount, lowY  * vertexSize + highX), fixedPointInverse);
    float heightLowHigh  = fixedPointToHeight(columnTotalFixedPoint(stratumCount, cellCount, highY * vertexSize + lowX),  fixedPointInverse);
    float heightHighHigh = fixedPointToHeight(columnTotalFixedPoint(stratumCount, cellCount, highY * vertexSize + highX), fixedPointInverse);

    float gradientX = (heightHighLow - heightLowLow) * (1.0 - fractionY) + (heightHighHigh - heightLowHigh) * fractionY;
    float gradientY = (heightLowHigh - heightLowLow) * (1.0 - fractionX) + (heightHighHigh - heightHighLow) * fractionX;
    float height = heightLowLow * (1.0 - fractionX) * (1.0 - fractionY) + heightHighLow * fractionX * (1.0 - fractionY)
                 + heightLowHigh * (1.0 - fractionX) * fractionY + heightHighHigh * fractionX * fractionY;
    return vec3(height, gradientX, gradientY);
}

int findTopMaterialStratum(int cellCount, int cellIndex, int highestStratum, int thicknessEpsilonTicks) {
    for (int stratum = highestStratum; stratum >= 0; --stratum)
        if (thicknessFixedPoint[stratum * cellCount + cellIndex] > thicknessEpsilonTicks) return stratum;
    return -1;
}

float materialPhysicsValue(int stratum, int offset) {
    return materialPhysics[stratum * MATERIAL_PHYSICS_STRIDE + offset];
}

// Race-free clamped subtract: compare-and-swap until the exchange wins, so a column can never
// be driven negative no matter how many droplets land on it in the same dispatch.
int erodeStratumClamped(int elementIndex, int requestedTicks) {
    int current = thicknessFixedPoint[elementIndex];
    for (int attempt = 0; attempt < EROSION_ATOMIC_RETRY_LIMIT; ++attempt) {
        int take = min(current, requestedTicks);
        if (take <= 0) return 0;
        int previous = atomicCompSwap(thicknessFixedPoint[elementIndex], current, current - take);
        if (previous == current) return take;
        current = previous;
    }
    return 0;
}

int erodeColumnClamped(int cellCount, int cellIndex, int highestStratum, int requestedTicks) {
    int remaining = requestedTicks;
    int removed = 0;
    for (int stratum = highestStratum; stratum >= 0 && remaining > 0; --stratum) {
        if (materialPhysicsValue(stratum, MATERIAL_PHYSICS_ERODABLE_OFFSET) <= 0.0) continue;
        int taken = erodeStratumClamped(stratum * cellCount + cellIndex, remaining);
        remaining -= taken;
        removed += taken;
    }
    return removed;
}

int depositColumn(int cellCount, int cellIndex, int stratum, int amountTicks) {
    if (amountTicks <= 0) return 0;
    atomicAdd(thicknessFixedPoint[stratum * cellCount + cellIndex], amountTicks);
    return amountTicks;
}

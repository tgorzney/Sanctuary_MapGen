#version 430 core

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer HeightMap {
    float heights[];
};
layout(std430, binding = 1) readonly buffer SlopeMap {
    float slopes[];
};
layout(std430, binding = 2) writeonly buffer OutMask {
    int outMask[];
};

uniform int mapSize;
uniform int halfSize;
uniform int symMask;

uniform float minGradSq;
uniform float maxGradSq;
uniform float minHeight;
uniform float maxHeight;

// Focus gradient parameters
uniform int focusGradientMode;
uniform float focusRadius;
uniform float focusContrast;
uniform float focusStrength;
uniform int seed;

// Symmetry constants
const int Symmetry_Point = 1;
const int Symmetry_X = 2;
const int Symmetry_Z = 4;
const int Symmetry_XY = 8;
const int Symmetry_Radial = 16;
const int Symmetry_Rotational = 32;

// Pseudo-random function for focus gradient stochastic rejection
uint pcg_hash(uint input) {
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= mapSize || pos.y >= mapSize) return;

    int idx = pos.y * mapSize + pos.x;
    outMask[idx] = 0; // Initialize as 0

    // Symmetry Culling (mirrors CPU logic)
    if ((symMask & Symmetry_Point) != 0 && pos.y > halfSize) return;
    if ((symMask & Symmetry_Z) != 0 && pos.y > halfSize) return;
    if ((symMask & Symmetry_Point) != 0 && pos.y == halfSize && pos.x > halfSize) return;
    if ((symMask & Symmetry_X) != 0 && pos.x > halfSize) return;
    if ((symMask & Symmetry_XY) != 0 && pos.x > pos.y) return;

    // Use fixed point deterministic comparison for heights and slopes by multiplying by 100000
    // This avoids IEEE-754 cross-vendor rounding drift.
    int iSlope = int(slopes[idx] * 100000.0);
    int iMinS = int(minGradSq * 100000.0);
    int iMaxS = int(maxGradSq * 100000.0);
    if (iSlope < iMinS || iSlope > iMaxS) return;

    int iHeight = int(heights[idx] * 10000.0);
    int iMinH = int(minHeight * 10000.0);
    int iMaxH = int(maxHeight * 10000.0);
    if (iHeight < iMinH || iHeight > iMaxH) return;

    // Focus Gradient Rejection
    if (focusGradientMode != 0 && focusRadius > 0.0) {
        float dx = float(pos.x - halfSize);
        float dy = float(pos.y - halfSize);
        float distVal = sqrt(dx*dx + dy*dy);
        float prob = 1.0;
        
        if (focusGradientMode == 1) { // CenterFocus
            prob = 1.0 - clamp(distVal / focusRadius, 0.0, 1.0);
        } else if (focusGradientMode == 2) { // EdgeFocus
            prob = clamp(distVal / focusRadius, 0.0, 1.0);
        } else if (focusGradientMode == 3) { // Torus
            float normDist = distVal / focusRadius;
            prob = 1.0 - 2.0 * abs(normDist - 0.5);
            prob = clamp(prob, 0.0, 1.0);
        }
        
        prob = pow(prob, focusContrast) * focusStrength;
        
        // 1000 scale stochastic rejection to maintain int exactness
        uint rejectSeed = seed ^ (pos.x * 38243u) ^ (pos.y * 94833u);
        uint hashed = pcg_hash(rejectSeed);
        int iRand = int(hashed % 1000u);
        int iProb = int(prob * 1000.0);
        
        if (iRand > iProb) return;
    }

    // Write 1 to output mask
    outMask[idx] = 1;
}

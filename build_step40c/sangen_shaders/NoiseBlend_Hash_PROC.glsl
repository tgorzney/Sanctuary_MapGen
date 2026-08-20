#version 430 core
// NoiseBlend_Hash_PROC.glsl — GPU half of the noise basis: FastNoiseLite's integer hash,
// value coordinate and gradient coordinate (2D), bit-for-bit equivalent to the CPU twin in
// NoiseBlend_Noise_PROC.cpp. Coordinate priming and hashing run in `uint` because unsigned
// wrap-around is defined in GLSL while signed overflow is not; the bit patterns (and so the
// results) match FastNoiseLite's wrapping int arithmetic exactly.
// One compilation unit of the NoiseBlend program (linked, never #included).

const uint noisePrimeX = 501125321u;
const uint noisePrimeY = 1136930381u;

int fastFloorToInt(float value)  { return value >= 0.0 ? int(value) : int(value) - 1; }
int fastRoundToInt(float value)  { return value >= 0.0 ? int(value + 0.5) : int(value - 0.5); }
float interpolateHermite(float t) { return t * t * (3.0 - 2.0 * t); }
float interpolateQuintic(float t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
float lerpValue(float a, float b, float t) { return a + t * (b - a); }
float cubicLerpValue(float a, float b, float c, float d, float t) {
    float p = (d - c) - (a - b);
    return t * t * t * p + t * t * ((a - b) - p) + t * (c - a) + b;
}

// Accessors so the primes live in exactly one compilation unit (GLSL has no shared consts).
uint primedCoordinateX(int cellX) { return uint(cellX) * noisePrimeX; }
uint primedCoordinateY(int cellY) { return uint(cellY) * noisePrimeY; }
uint primeStepX() { return noisePrimeX; }
uint primeStepY() { return noisePrimeY; }

uint noiseHash(int seed, uint primedX, uint primedY) {
    uint hash = uint(seed) ^ primedX ^ primedY;
    return hash * 0x27d4eb2du;
}

float valueCoordinate(int seed, uint primedX, uint primedY) {
    uint hash = noiseHash(seed, primedX, primedY);
    hash *= hash;
    hash ^= hash << 19;
    return float(int(hash)) * (1.0 / 2147483648.0);
}

// FastNoiseLite's 128 2D gradients are the 24-direction cycle repeated five times followed
// by the eight 45-degree directions — stored as the cycle plus the tail instead of a
// 256-float table (identical values, a fraction of the file).
const vec2 gradientCycle[24] = vec2[24](
    vec2(0.130526192220052,0.99144486137381), vec2(0.38268343236509,0.923879532511287),
    vec2(0.608761429008721,0.793353340291235), vec2(0.793353340291235,0.608761429008721),
    vec2(0.923879532511287,0.38268343236509), vec2(0.99144486137381,0.130526192220051),
    vec2(0.99144486137381,-0.130526192220051), vec2(0.923879532511287,-0.38268343236509),
    vec2(0.793353340291235,-0.60876142900872), vec2(0.608761429008721,-0.793353340291235),
    vec2(0.38268343236509,-0.923879532511287), vec2(0.130526192220052,-0.99144486137381),
    vec2(-0.130526192220052,-0.99144486137381), vec2(-0.38268343236509,-0.923879532511287),
    vec2(-0.608761429008721,-0.793353340291235), vec2(-0.793353340291235,-0.608761429008721),
    vec2(-0.923879532511287,-0.38268343236509), vec2(-0.99144486137381,-0.130526192220052),
    vec2(-0.99144486137381,0.130526192220051), vec2(-0.923879532511287,0.38268343236509),
    vec2(-0.793353340291235,0.608761429008721), vec2(-0.608761429008721,0.793353340291235),
    vec2(-0.38268343236509,0.923879532511287), vec2(-0.130526192220052,0.99144486137381));
const vec2 gradientTail[8] = vec2[8](
    vec2(0.38268343236509,0.923879532511287), vec2(0.923879532511287,0.38268343236509),
    vec2(0.923879532511287,-0.38268343236509), vec2(0.38268343236509,-0.923879532511287),
    vec2(-0.38268343236509,-0.923879532511287), vec2(-0.923879532511287,-0.38268343236509),
    vec2(-0.923879532511287,0.38268343236509), vec2(-0.38268343236509,0.923879532511287));

vec2 gradientVector(int gradientIndex) {
    return gradientIndex < 120 ? gradientCycle[gradientIndex % 24] : gradientTail[gradientIndex - 120];
}

float gradientCoordinate(int seed, uint primedX, uint primedY, float deltaX, float deltaY) {
    uint hash = noiseHash(seed, primedX, primedY);
    hash ^= hash >> 15;
    vec2 gradient = gradientVector(int((hash & uint(127 << 1)) >> 1));
    return deltaX * gradient.x + deltaY * gradient.y;
}

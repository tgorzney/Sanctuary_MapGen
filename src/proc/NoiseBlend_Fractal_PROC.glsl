#version 430 core
// NoiseBlend_Fractal_PROC.glsl — noise-type selection, the coordinate transform, and the
// fractal octave loop (FractionalBrownian / Ridged / PingPong), ported from FastNoiseLite's
// GenNoiseSingle + TransformNoiseCoordinate + GenFractal*. The enum values come in as
// #defines built from the C++ enums themselves, so the two sides cannot drift.
// One compilation unit of the NoiseBlend program (linked, never #included).

float openSimplex2(int seed, float pointX, float pointY);
float openSimplex2Smooth(int seed, float pointX, float pointY);
float cellularNoise(int seed, float pointX, float pointY, float jitterModifier);
float perlinNoise(int seed, float pointX, float pointY);
float valueNoise(int seed, float pointX, float pointY);
float valueCubicNoise(int seed, float pointX, float pointY);

const float simplexSkew = 0.36602540378443864;   // F2 = 0.5 * (sqrt(3) - 1)

float singleNoise(int noiseType, int seed, vec2 point, float cellularJitter) {
    if (noiseType == NOISE_TYPE_OPEN_SIMPLEX2)        return openSimplex2(seed, point.x, point.y);
    if (noiseType == NOISE_TYPE_OPEN_SIMPLEX2_SMOOTH) return openSimplex2Smooth(seed, point.x, point.y);
    if (noiseType == NOISE_TYPE_CELLULAR)             return cellularNoise(seed, point.x, point.y, cellularJitter);
    if (noiseType == NOISE_TYPE_PERLIN)               return perlinNoise(seed, point.x, point.y);
    if (noiseType == NOISE_TYPE_VALUE_CUBIC)          return valueCubicNoise(seed, point.x, point.y);
    if (noiseType == NOISE_TYPE_VALUE)                return valueNoise(seed, point.x, point.y);
    return 0.0;
}

// Frequency scaling plus the simplex skew — applied ONCE before the octave loop, exactly as
// FastNoiseLite does it (the loop then walks the coordinates by lacunarity).
vec2 transformNoisePoint(vec2 point, float frequency, int noiseType) {
    vec2 scaled = point * frequency;
    if (noiseType == NOISE_TYPE_OPEN_SIMPLEX2 || noiseType == NOISE_TYPE_OPEN_SIMPLEX2_SMOOTH) {
        float skew = (scaled.x + scaled.y) * simplexSkew;
        scaled += vec2(skew, skew);
    }
    return scaled;
}

float pingPongWave(float t) {
    t -= float(int(t * 0.5)) * 2.0;
    return t < 1.0 ? t : 2.0 - t;
}

float fractalNoise(int noiseType, int fractalType, int seed, int octaves, float gain, float lacunarity,
                   float weightedStrength, float pingPongStrength, float cellularJitter,
                   float fractalBounding, vec2 point) {
    if (fractalType == FRACTAL_TYPE_NONE) return singleNoise(noiseType, seed, point, cellularJitter);

    float sum = 0.0;
    float amplitude = fractalBounding;
    vec2 walkingPoint = point;
    int octaveSeed = seed;
    for (int octave = 0; octave < octaves; ++octave) {
        float noiseValue = singleNoise(noiseType, octaveSeed, walkingPoint, cellularJitter);
        if (fractalType == FRACTAL_TYPE_RIDGED) {
            noiseValue = abs(noiseValue);
            sum += (noiseValue * -2.0 + 1.0) * amplitude;
            amplitude *= mix(1.0, 1.0 - noiseValue, weightedStrength);
        } else if (fractalType == FRACTAL_TYPE_PING_PONG) {
            noiseValue = pingPongWave((noiseValue + 1.0) * pingPongStrength);
            sum += (noiseValue - 0.5) * 2.0 * amplitude;
            amplitude *= mix(1.0, noiseValue, weightedStrength);
        } else {
            sum += noiseValue * amplitude;
            amplitude *= mix(1.0, min(noiseValue + 1.0, 2.0) * 0.5, weightedStrength);
        }
        walkingPoint *= lacunarity;
        amplitude *= gain;
        octaveSeed += 1;
    }
    return sum;
}

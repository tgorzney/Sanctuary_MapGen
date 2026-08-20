#version 430 core
// NoiseBlend_Simplex_PROC.glsl — OpenSimplex2 (2D), ported from FastNoiseLite's
// SingleSimplex so the Gpu speed path produces the same shape as the Cpu accuracy path.
// The skew is applied once by the caller (transformNoisePoint), exactly as FastNoiseLite
// moves it into TransformNoiseCoordinate. One compilation unit of the NoiseBlend program.

int fastFloorToInt(float value);
uint primedCoordinateX(int cellX);
uint primedCoordinateY(int cellY);
uint primeStepX();
uint primeStepY();
float gradientCoordinate(int seed, uint primedX, uint primedY, float deltaX, float deltaY);

// Skew/unskew constants of the 2D simplex lattice, written as decimal literals so both
// backends see the same rounded float (FastNoiseLite folds the same expressions).
const float simplexUnskew          = 0.21132486540518711775;   // G2 = (3 - sqrt(3)) / 6
const float simplexFalloffSlope    = 3.1547005383792515;       // 2 * (1 - 2G2) * (1/G2 - 2)
const float simplexFalloffOffset   = -0.66666666666666666;     // -2 * (1 - 2G2)^2
const float simplexCornerOffset    = -0.5773502691896258;      // 2G2 - 1
const float simplexNormalizer      = 99.83685446303647;

float openSimplex2(int seed, float pointX, float pointY) {
    int cellX = fastFloorToInt(pointX);
    int cellY = fastFloorToInt(pointY);
    float insideX = pointX - float(cellX);
    float insideY = pointY - float(cellY);

    float unskew = (insideX + insideY) * simplexUnskew;
    float deltaX0 = insideX - unskew;
    float deltaY0 = insideY - unskew;
    uint primedX = primedCoordinateX(cellX);
    uint primedY = primedCoordinateY(cellY);

    float corner0 = 0.0;
    float corner1 = 0.0;
    float corner2 = 0.0;

    float falloff0 = 0.5 - deltaX0 * deltaX0 - deltaY0 * deltaY0;
    if (falloff0 > 0.0) {
        float squared = falloff0 * falloff0;
        corner0 = squared * squared * gradientCoordinate(seed, primedX, primedY, deltaX0, deltaY0);
    }

    float falloff2 = simplexFalloffSlope * unskew + (simplexFalloffOffset + falloff0);
    if (falloff2 > 0.0) {
        float deltaX2 = deltaX0 + simplexCornerOffset;
        float deltaY2 = deltaY0 + simplexCornerOffset;
        float squared = falloff2 * falloff2;
        corner2 = squared * squared * gradientCoordinate(seed, primedX + primeStepX(),
                                                         primedY + primeStepY(), deltaX2, deltaY2);
    }

    if (deltaY0 > deltaX0) {
        float deltaX1 = deltaX0 + simplexUnskew;
        float deltaY1 = deltaY0 + (simplexUnskew - 1.0);
        float falloff1 = 0.5 - deltaX1 * deltaX1 - deltaY1 * deltaY1;
        if (falloff1 > 0.0) {
            float squared = falloff1 * falloff1;
            corner1 = squared * squared * gradientCoordinate(seed, primedX, primedY + primeStepY(),
                                                             deltaX1, deltaY1);
        }
    } else {
        float deltaX1 = deltaX0 + (simplexUnskew - 1.0);
        float deltaY1 = deltaY0 + simplexUnskew;
        float falloff1 = 0.5 - deltaX1 * deltaX1 - deltaY1 * deltaY1;
        if (falloff1 > 0.0) {
            float squared = falloff1 * falloff1;
            corner1 = squared * squared * gradientCoordinate(seed, primedX + primeStepX(), primedY,
                                                             deltaX1, deltaY1);
        }
    }
    return (corner0 + corner1 + corner2) * simplexNormalizer;
}

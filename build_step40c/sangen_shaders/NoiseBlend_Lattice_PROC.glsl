#version 430 core
// NoiseBlend_Lattice_PROC.glsl — the grid-lattice noise family (2D): Perlin, Value and
// ValueCubic, ported from FastNoiseLite's SinglePerlin / SingleValue / SingleValueCubic.
// One compilation unit of the NoiseBlend program (linked, never #included).

int fastFloorToInt(float value);
float interpolateHermite(float t);
float interpolateQuintic(float t);
float lerpValue(float a, float b, float t);
float cubicLerpValue(float a, float b, float c, float d, float t);
uint primedCoordinateX(int cellX);
uint primedCoordinateY(int cellY);
uint primeStepX();
uint primeStepY();
float valueCoordinate(int seed, uint primedX, uint primedY);
float gradientCoordinate(int seed, uint primedX, uint primedY, float deltaX, float deltaY);

const float perlinNormalizer    = 1.4247691104677813;
const float valueCubicNormalizer = 1.0 / (1.5 * 1.5);

float perlinNoise(int seed, float pointX, float pointY) {
    int cellX = fastFloorToInt(pointX);
    int cellY = fastFloorToInt(pointY);
    float deltaX0 = pointX - float(cellX);
    float deltaY0 = pointY - float(cellY);
    float deltaX1 = deltaX0 - 1.0;
    float deltaY1 = deltaY0 - 1.0;
    float weightX = interpolateQuintic(deltaX0);
    float weightY = interpolateQuintic(deltaY0);

    uint primedX0 = primedCoordinateX(cellX);
    uint primedY0 = primedCoordinateY(cellY);
    uint primedX1 = primedX0 + primeStepX();
    uint primedY1 = primedY0 + primeStepY();

    float lowerRow = lerpValue(gradientCoordinate(seed, primedX0, primedY0, deltaX0, deltaY0),
                               gradientCoordinate(seed, primedX1, primedY0, deltaX1, deltaY0), weightX);
    float upperRow = lerpValue(gradientCoordinate(seed, primedX0, primedY1, deltaX0, deltaY1),
                               gradientCoordinate(seed, primedX1, primedY1, deltaX1, deltaY1), weightX);
    return lerpValue(lowerRow, upperRow, weightY) * perlinNormalizer;
}

float valueNoise(int seed, float pointX, float pointY) {
    int cellX = fastFloorToInt(pointX);
    int cellY = fastFloorToInt(pointY);
    float weightX = interpolateHermite(pointX - float(cellX));
    float weightY = interpolateHermite(pointY - float(cellY));

    uint primedX0 = primedCoordinateX(cellX);
    uint primedY0 = primedCoordinateY(cellY);
    uint primedX1 = primedX0 + primeStepX();
    uint primedY1 = primedY0 + primeStepY();

    float lowerRow = lerpValue(valueCoordinate(seed, primedX0, primedY0),
                               valueCoordinate(seed, primedX1, primedY0), weightX);
    float upperRow = lerpValue(valueCoordinate(seed, primedX0, primedY1),
                               valueCoordinate(seed, primedX1, primedY1), weightX);
    return lerpValue(lowerRow, upperRow, weightY);
}

float valueCubicNoise(int seed, float pointX, float pointY) {
    int cellX = fastFloorToInt(pointX);
    int cellY = fastFloorToInt(pointY);
    float weightX = pointX - float(cellX);
    float weightY = pointY - float(cellY);

    uint primedX1 = primedCoordinateX(cellX);
    uint primedY1 = primedCoordinateY(cellY);
    uint primedX0 = primedX1 - primeStepX();
    uint primedY0 = primedY1 - primeStepY();
    uint primedX2 = primedX1 + primeStepX();
    uint primedY2 = primedY1 + primeStepY();
    uint primedX3 = primedX1 + (primeStepX() << 1);
    uint primedY3 = primedY1 + (primeStepY() << 1);

    return cubicLerpValue(
        cubicLerpValue(valueCoordinate(seed, primedX0, primedY0), valueCoordinate(seed, primedX1, primedY0),
                       valueCoordinate(seed, primedX2, primedY0), valueCoordinate(seed, primedX3, primedY0), weightX),
        cubicLerpValue(valueCoordinate(seed, primedX0, primedY1), valueCoordinate(seed, primedX1, primedY1),
                       valueCoordinate(seed, primedX2, primedY1), valueCoordinate(seed, primedX3, primedY1), weightX),
        cubicLerpValue(valueCoordinate(seed, primedX0, primedY2), valueCoordinate(seed, primedX1, primedY2),
                       valueCoordinate(seed, primedX2, primedY2), valueCoordinate(seed, primedX3, primedY2), weightX),
        cubicLerpValue(valueCoordinate(seed, primedX0, primedY3), valueCoordinate(seed, primedX1, primedY3),
                       valueCoordinate(seed, primedX2, primedY3), valueCoordinate(seed, primedX3, primedY3), weightX),
        weightY) * valueCubicNormalizer;
}

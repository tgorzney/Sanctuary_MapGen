#version 430 core
// NoiseBlend_SimplexSmooth_PROC.glsl — OpenSimplex2S (2D), ported from FastNoiseLite's
// SingleOpenSimplex2S. Same lattice as OpenSimplex2 with the smoother 2/3 falloff and two
// extra corners chosen by nested conditionals. Constants and the local lattice record carry
// a unit-local prefix so no two compilation units declare the same global name.
// One compilation unit of the NoiseBlend program (linked, never #included).

int fastFloorToInt(float value);
uint primedCoordinateX(int cellX);
uint primedCoordinateY(int cellY);
uint primeStepX();
uint primeStepY();
float gradientCoordinate(int seed, uint primedX, uint primedY, float deltaX, float deltaY);

const float smoothUnskew        = 0.21132486540518711775;   // G2
const float smoothFalloffSlope  = 3.1547005383792515;       // 2 * (1 - 2G2) * (1/G2 - 2)
const float smoothFalloffOffset = -0.66666666666666666;     // -2 * (1 - 2G2)^2
const float smoothCornerShift   = 0.5773502691896258;       // 1 - 2G2
const float smoothFalloffBase   = 0.66666666666666666;      // 2/3
const float smoothNormalizer    = 18.24196194486065;

struct SmoothLattice {
    uint  primedX;  uint  primedY;  uint primedX1; uint primedY1;
    float insideX;  float insideY;  float deltaX0; float deltaY0;
};

// One extra lattice corner; contributes nothing once the falloff goes negative.
float smoothCornerContribution(int seed, uint primedX, uint primedY, float deltaX, float deltaY) {
    float falloff = smoothFalloffBase - deltaX * deltaX - deltaY * deltaY;
    if (falloff <= 0.0) return 0.0;
    float squared = falloff * falloff;
    return squared * squared * gradientCoordinate(seed, primedX, primedY, deltaX, deltaY);
}

// The two extra corners when the sample sits in the upper simplex (unskew > G2).
float smoothUpperCorners(int seed, SmoothLattice lattice, float insideDifference) {
    float total = (lattice.insideX + insideDifference > 1.0)
        ? smoothCornerContribution(seed, lattice.primedX + (primeStepX() << 1), lattice.primedY1,
                                   lattice.deltaX0 + (3.0 * smoothUnskew - 2.0),
                                   lattice.deltaY0 + (3.0 * smoothUnskew - 1.0))
        : smoothCornerContribution(seed, lattice.primedX, lattice.primedY1,
                                   lattice.deltaX0 + smoothUnskew, lattice.deltaY0 + (smoothUnskew - 1.0));
    total += (lattice.insideY - insideDifference > 1.0)
        ? smoothCornerContribution(seed, lattice.primedX1, lattice.primedY + (primeStepY() << 1),
                                   lattice.deltaX0 + (3.0 * smoothUnskew - 1.0),
                                   lattice.deltaY0 + (3.0 * smoothUnskew - 2.0))
        : smoothCornerContribution(seed, lattice.primedX1, lattice.primedY,
                                   lattice.deltaX0 + (smoothUnskew - 1.0), lattice.deltaY0 + smoothUnskew);
    return total;
}

// The two extra corners when the sample sits in the lower simplex.
float smoothLowerCorners(int seed, SmoothLattice lattice, float insideDifference) {
    float total = (lattice.insideX + insideDifference < 0.0)
        ? smoothCornerContribution(seed, lattice.primedX - primeStepX(), lattice.primedY,
                                   lattice.deltaX0 + (1.0 - smoothUnskew), lattice.deltaY0 - smoothUnskew)
        : smoothCornerContribution(seed, lattice.primedX1, lattice.primedY,
                                   lattice.deltaX0 + (smoothUnskew - 1.0), lattice.deltaY0 + smoothUnskew);
    total += (lattice.insideY < insideDifference)
        ? smoothCornerContribution(seed, lattice.primedX, lattice.primedY - primeStepY(),
                                   lattice.deltaX0 - smoothUnskew, lattice.deltaY0 - (smoothUnskew - 1.0))
        : smoothCornerContribution(seed, lattice.primedX, lattice.primedY1,
                                   lattice.deltaX0 + smoothUnskew, lattice.deltaY0 + (smoothUnskew - 1.0));
    return total;
}

float openSimplex2Smooth(int seed, float pointX, float pointY) {
    int cellX = fastFloorToInt(pointX);
    int cellY = fastFloorToInt(pointY);
    SmoothLattice lattice;
    lattice.insideX = pointX - float(cellX);
    lattice.insideY = pointY - float(cellY);
    lattice.primedX = primedCoordinateX(cellX);
    lattice.primedY = primedCoordinateY(cellY);
    lattice.primedX1 = lattice.primedX + primeStepX();
    lattice.primedY1 = lattice.primedY + primeStepY();

    float unskew = (lattice.insideX + lattice.insideY) * smoothUnskew;
    lattice.deltaX0 = lattice.insideX - unskew;
    lattice.deltaY0 = lattice.insideY - unskew;

    float falloff0 = smoothFalloffBase - lattice.deltaX0 * lattice.deltaX0 - lattice.deltaY0 * lattice.deltaY0;
    float squared0 = falloff0 * falloff0;
    float value = squared0 * squared0
                * gradientCoordinate(seed, lattice.primedX, lattice.primedY, lattice.deltaX0, lattice.deltaY0);

    float falloff1 = smoothFalloffSlope * unskew + (smoothFalloffOffset + falloff0);
    float squared1 = falloff1 * falloff1;
    value += squared1 * squared1 * gradientCoordinate(seed, lattice.primedX1, lattice.primedY1,
                                                      lattice.deltaX0 - smoothCornerShift,
                                                      lattice.deltaY0 - smoothCornerShift);

    float insideDifference = lattice.insideX - lattice.insideY;
    value += (unskew > smoothUnskew) ? smoothUpperCorners(seed, lattice, insideDifference)
                                     : smoothLowerCorners(seed, lattice, insideDifference);
    return value * smoothNormalizer;
}

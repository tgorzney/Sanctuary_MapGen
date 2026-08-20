#version 430 core
// Erosion_PROC.glsl — Gpu speed path of the hydraulic erosion stage; twin of the Cpu pair
// Erosion_Droplet_PROC.cpp + Erosion_DropletTransfer_PROC.cpp, expression for expression. One
// droplet per invocation over the fixed-point stack (Erosion_Column_PROC.glsl), transfers in
// Erosion_Splat_PROC.glsl. Nothing is hardcoded: every rate arrives in the std430
// ErosionConfiguration block mirroring Proc::ErosionKernelConfiguration field for field
// (Constitution §8 — where the old shader's erosion 0.3 died); the meander/divergence term the
// old Gpu path lacked runs here too, off the same hash stream as the Cpu.
layout(local_size_x = EROSION_WORKGROUP_SIZE) in;

struct ErosionConfiguration {
    int dropletCount;            int maximumLifetime;           int vertexSize;               int stratumCount;
    int depositStratum;          int highestErodableStratum;    int bDepositionMode;          int randomSeed;
    int bConserveSedimentAtExit; int integerPadding;
    float heightFixedPointScale; float heightFixedPointInverse; float baseErosionRate;        float baseDepositionRate;
    float evaporationRate;       float gravity;                 float capacityBaseMultiplier; float carryingCapacityScale;
    float capacityMinimum;       float inertiaBase;             float inertiaFrictionScale;   float viscosityReciprocal;
    float meanderStrength;       float divergenceFactor;        float divergenceThreshold;    float waterMinimum;
    float sedimentMinimum;       float thicknessEpsilon;        float initialSedimentLoad;    float boundaryMargin;
    float depositionModeCapacityGain; float floatPadding;
};
struct Droplet { vec2 position; vec2 direction; float speed; float water; float sediment; uint seed; };
layout(std430, binding = 0) readonly buffer ErosionConfigurations { ErosionConfiguration configurations[]; };
layout(std430, binding = 2) readonly buffer DropletSpawns { float dropletSpawns[]; };

// Provided by Erosion_Column_PROC.glsl / Erosion_Splat_PROC.glsl (same program, other units).
uint  hashRandomCombine(uint first, uint second);          float hashRandomUnitFloat(uint state);
int   heightToFixedPoint(float height, float fixedPointScale); float materialPhysicsValue(int stratum, int offset);
vec3  sampleColumnHeight(int stratumCount, int vertexSize, float fixedPointInverse, float sampleX, float sampleY);
int   findTopMaterialStratum(int cellCount, int cellIndex, int highestStratum, int thicknessEpsilonTicks);
float depositSplat(int cellCount, int vertexSize, int depositStratum, float fixedPointScale,
                   float fixedPointInverse, int nodeX, int nodeY, float fractionX, float fractionY, float amountHeight);
float erodeSplat(int cellCount, int vertexSize, int highestErodableStratum, float fixedPointScale,
                 float fixedPointInverse, int nodeX, int nodeY, float fractionX, float fractionY, float amountHeight);
void  settleDroplet(int cellCount, int vertexSize, int depositStratum, int bConserveSedimentAtExit,
                    float sedimentMinimum, float fixedPointScale, float sediment, int nodeX, int nodeY);

void settle(ErosionConfiguration configuration, int cellCount, float sediment, int nodeX, int nodeY) {
    settleDroplet(cellCount, configuration.vertexSize, configuration.depositStratum,
                  configuration.bConserveSedimentAtExit, configuration.sedimentMinimum,
                  configuration.heightFixedPointScale, sediment, nodeX, nodeY);
}

vec2 steerDirection(ErosionConfiguration configuration, vec2 direction, vec2 gradient, float friction, uint stepSeed) {
    float slopeLength = sqrt(gradient.x * gradient.x + gradient.y * gradient.y);
    float divergence = configuration.divergenceFactor
                     * (1.0 - clamp(slopeLength * configuration.divergenceThreshold, 0.0, 1.0));
    vec2 steer = gradient + vec2(hashRandomUnitFloat(stepSeed)      - 0.5,
                                 hashRandomUnitFloat(stepSeed + 1u) - 0.5) * configuration.meanderStrength * divergence;
    float inertia = (configuration.inertiaBase + (1.0 - friction) * configuration.inertiaFrictionScale)
                  * configuration.viscosityReciprocal;
    vec2 blended = direction * inertia - steer * (1.0 - inertia);
    float blendedLength = sqrt(blended.x * blended.x + blended.y * blended.y);
    return blendedLength != 0.0 ? blended * (1.0 / blendedLength) : blended;
}

float exchangeSediment(ErosionConfiguration configuration, int cellCount, float sediment, float deltaHeight,
                       float capacity, float hardness, int nodeX, int nodeY, float fractionX, float fractionY) {
    if (sediment > capacity || deltaHeight > 0.0) {
        float requested = deltaHeight > 0.0 ? min(deltaHeight, sediment)
                                            : (sediment - capacity) * configuration.baseDepositionRate;
        if (requested > 0.0)
            sediment -= depositSplat(cellCount, configuration.vertexSize, configuration.depositStratum,
                                     configuration.heightFixedPointScale, configuration.heightFixedPointInverse,
                                     nodeX, nodeY, fractionX, fractionY, requested);
    } else if (configuration.bDepositionMode == 0) {
        float requested = min((capacity - sediment) * configuration.baseErosionRate * (1.0 - hardness), -deltaHeight);
        if (requested > 0.0)
            sediment += erodeSplat(cellCount, configuration.vertexSize, configuration.highestErodableStratum,
                                   configuration.heightFixedPointScale, configuration.heightFixedPointInverse,
                                   nodeX, nodeY, fractionX, fractionY, requested);
    }
    return sediment;
}

// Sediment the water may carry; deposition mode holds its load while sliding down.
float dropletCapacity(ErosionConfiguration configuration, Droplet drop, float deltaHeight,
                      float materialCapacityMultiplier) {
    float capacity = max(-deltaHeight * drop.speed * drop.water * configuration.capacityBaseMultiplier
                         * materialCapacityMultiplier * configuration.carryingCapacityScale,
                         configuration.capacityMinimum);
    if (configuration.bDepositionMode != 0)
        capacity = max(capacity, drop.sediment * clamp(-deltaHeight * configuration.depositionModeCapacityGain, 0.0, 1.0));
    return capacity;
}

// The material the droplet is standing on; an empty column falls back to the deposit stratum.
int dropletPhysicsStratum(ErosionConfiguration configuration, int cellCount, int cellIndex) {
    int topStratum = findTopMaterialStratum(cellCount, cellIndex, configuration.highestErodableStratum,
                                            heightToFixedPoint(configuration.thicknessEpsilon,
                                                               configuration.heightFixedPointScale));
    return topStratum >= 0 ? topStratum : configuration.depositStratum;
}

// One droplet step; false means the droplet died (its load is already settled).
bool advanceDroplet(ErosionConfiguration configuration, int cellCount, inout Droplet drop, int step) {
    int nodeX = int(drop.position.x), nodeY = int(drop.position.y);
    vec2 fraction = drop.position - vec2(float(nodeX), float(nodeY));
    vec3 sampled = sampleColumnHeight(configuration.stratumCount, configuration.vertexSize,
                                      configuration.heightFixedPointInverse, drop.position.x, drop.position.y);
    int physicsStratum = dropletPhysicsStratum(configuration, cellCount, nodeY * configuration.vertexSize + nodeX);

    drop.direction = steerDirection(configuration, drop.direction, sampled.yz,
                                    materialPhysicsValue(physicsStratum, MATERIAL_PHYSICS_FRICTION_OFFSET),
                                    hashRandomCombine(drop.seed, uint(step)));
    drop.position += drop.direction;
    float highestCoordinate = float(configuration.vertexSize - 2);
    if ((drop.direction.x == 0.0 && drop.direction.y == 0.0)
        || drop.position.x < configuration.boundaryMargin || drop.position.x >= highestCoordinate
        || drop.position.y < configuration.boundaryMargin || drop.position.y >= highestCoordinate) {
        settle(configuration, cellCount, drop.sediment, nodeX, nodeY);
        return false;
    }

    vec3 moved = sampleColumnHeight(configuration.stratumCount, configuration.vertexSize,
                                    configuration.heightFixedPointInverse, drop.position.x, drop.position.y);
    float deltaHeight = moved.x - sampled.x;
    float capacity = dropletCapacity(configuration, drop, deltaHeight,
                                     materialPhysicsValue(physicsStratum, MATERIAL_PHYSICS_CAPACITY_OFFSET));
    drop.sediment = exchangeSediment(configuration, cellCount, drop.sediment, deltaHeight, capacity,
                                     materialPhysicsValue(physicsStratum, MATERIAL_PHYSICS_HARDNESS_OFFSET),
                                     nodeX, nodeY, fraction.x, fraction.y);

    drop.speed = sqrt(max(0.0, drop.speed * drop.speed + deltaHeight * configuration.gravity));
    drop.water *= (1.0 - configuration.evaporationRate)
                * (1.0 - materialPhysicsValue(physicsStratum, MATERIAL_PHYSICS_ABSORPTION_OFFSET));
    if (drop.water <= configuration.waterMinimum) {
        settle(configuration, cellCount, drop.sediment, nodeX, nodeY);
        return false;
    }
    return true;
}

void main() {
    ErosionConfiguration configuration = configurations[0];
    int dropletIndex = int(gl_GlobalInvocationID.x);
    if (dropletIndex >= configuration.dropletCount) return;
    int cellCount = configuration.vertexSize * configuration.vertexSize;

    Droplet drop;
    drop.position  = vec2(dropletSpawns[dropletIndex * 2], dropletSpawns[dropletIndex * 2 + 1]);
    drop.direction = vec2(0.0);
    drop.speed     = 1.0;
    drop.water     = 1.0;
    drop.sediment  = configuration.initialSedimentLoad;
    drop.seed      = hashRandomCombine(uint(configuration.randomSeed), uint(dropletIndex));
    for (int step = 0; step < configuration.maximumLifetime; ++step)
        if (!advanceDroplet(configuration, cellCount, drop, step)) return;
    settle(configuration, cellCount, drop.sediment, int(drop.position.x), int(drop.position.y));
}

#version 430 core
// Placement_PROC.glsl — GPU preview density gate; twin of the Cpu gate in
// Placement_Fields_PROC.cpp. It evaluates ONE rule's per-cell gate weight (height, slope,
// biome mask, obstacle distance, water, edge padding, focus gradient) into a weight field.
// It deliberately does NOT scatter: placement stays Cpu-authoritative and the Cpu acceptance
// samples this field, so the preview can never re-filter its own version of the map
// (PLACEMENT_SCATTER_SPEC "CPU vs GPU", ARCH §3.2). Every tile size and flag bit arrives as
// a #define built from the C++ constants — nothing is hardcoded here (Constitution §8).
layout(local_size_x = PLACEMENT_TILE_WIDTH, local_size_y = PLACEMENT_TILE_HEIGHT) in;

// Mirrors Proc::ScatterRuleConfiguration field for field (DISPATCH_INTERFACE_SPEC §4).
struct ScatterRuleConfiguration {
    int   ruleIndex;         int   collectionIndex;   int   category;          int   priorityMode;
    int   focusGradientMode; int   symmetryMask;      int   maskStratumIndex;  int   targetCount;
    int   mapEdgePadding;    int   selectionFlags;    int   armyIndex;         int   ruleSeed;
    float density;           float heightMinimum;     float heightMaximum;
    float slopeGradientMinimumSquared; float slopeGradientMaximumSquared;
    float spacingMinimum;    float clearanceRadiusMinimum; float clearanceRadiusMaximum;
    float clearanceHeightTolerance;   float maskWeightMinimum;
    float obstacleDistanceMinimum;    float nearCliffDistanceMaximum;
    float focusGradientRadiusReciprocal; float focusGradientStrength; float focusGradientContrast;
    float scaleMinimum;      float scaleMaximum;
    float rotationMinimumRadians;     float rotationMaximumRadians;
    float waterSurfaceNormalized;
};

layout(std430, binding = 0) readonly buffer RuleConfigurations { ScatterRuleConfiguration rules[]; };
layout(std430, binding = 1) readonly buffer HeightField        { float heightValues[]; };
layout(std430, binding = 2) readonly buffer SlopeGradientField { float slopeGradientValues[]; };
layout(std430, binding = 3) readonly buffer StratumWeightField { float surfaceWeightValues[]; };
layout(std430, binding = 4) readonly buffer ObstacleField      { float obstacleValues[]; };
layout(std430, binding = 5) writeonly buffer GateWeightField   { float gateWeights[]; };

uniform int vertexSize;
uniform int bMaskFieldPresent;
uniform int bObstacleFieldPresent;

// Mirrors FocusGradientWeight in Placement_Gate_PROC.h — a rational gamma, never pow().
float focusGradientWeight(ScatterRuleConfiguration rule, float focusDistance) {
    if (rule.focusGradientMode == 0 || rule.focusGradientStrength <= 0.0) return 1.0;
    float normalizedDistance = focusDistance * rule.focusGradientRadiusReciprocal;
    float base;
    if (rule.focusGradientMode == 1)      base = 1.0 - clamp(normalizedDistance, 0.0, 1.0);
    else if (rule.focusGradientMode == 2) base = clamp(normalizedDistance, 0.0, 1.0);
    else                                  base = 1.0 - clamp(abs(normalizedDistance - 1.0), 0.0, 1.0);
    float denominator = base + (1.0 - base) * rule.focusGradientContrast;
    float shaped = denominator > 1e-6 ? base / denominator : base;
    return 1.0 - rule.focusGradientStrength + rule.focusGradientStrength * shaped;
}

// Mirrors ScatterGateWeight in Placement_Gate_PROC.h, expression for expression.
float scatterGateWeight(ScatterRuleConfiguration rule, float heightNormalized, float slopeGradientSquared,
                        float maskWeight, float obstacleDistance, float focusDistance) {
    if (heightNormalized < rule.heightMinimum) return 0.0;
    if (heightNormalized > rule.heightMaximum) return 0.0;
    if (slopeGradientSquared < rule.slopeGradientMinimumSquared) return 0.0;
    if (slopeGradientSquared > rule.slopeGradientMaximumSquared) return 0.0;
    if (rule.maskStratumIndex >= 0 && maskWeight < rule.maskWeightMinimum) return 0.0;
    if (rule.obstacleDistanceMinimum > 0.0 && obstacleDistance < rule.obstacleDistanceMinimum) return 0.0;
    if ((rule.selectionFlags & PLACEMENT_FLAG_NEAR_CLIFFS) != 0
        && obstacleDistance > rule.nearCliffDistanceMaximum) return 0.0;
    if ((rule.selectionFlags & PLACEMENT_FLAG_AVOID_WATER) != 0
        && heightNormalized <= rule.waterSurfaceNormalized) return 0.0;
    return focusGradientWeight(rule, focusDistance);
}

void main() {
    ivec2 cell = ivec2(gl_GlobalInvocationID.xy);
    if (cell.x >= vertexSize || cell.y >= vertexSize) return;
    int index = cell.y * vertexSize + cell.x;
    ScatterRuleConfiguration rule = rules[0];

    int padding = rule.mapEdgePadding;
    if (cell.x < padding || cell.y < padding
        || cell.x >= vertexSize - padding || cell.y >= vertexSize - padding) {
        gateWeights[index] = 0.0;
        return;
    }
    float mapCenter = float(vertexSize - 1) * 0.5;
    vec2 offset = vec2(float(cell.x) - mapCenter, float(cell.y) - mapCenter);
    float focusDistance = sqrt(dot(offset, offset));
    float maskWeight = bMaskFieldPresent != 0 ? surfaceWeightValues[index] : 1.0;
    float obstacleDistance = bObstacleFieldPresent != 0 ? obstacleValues[index]
                                                        : PLACEMENT_OBSTACLE_DISTANCE_DEFAULT;
    gateWeights[index] = scatterGateWeight(rule, heightValues[index], slopeGradientValues[index],
                                           maskWeight, obstacleDistance, focusDistance);
}

// Erosion_Droplet_PROC.h — the droplet trace contract (Cpu side of the Mei-style hydraulic).
// Layer: PROC. A droplet is a tiny value type plus a view of the fixed-point stack, so the
// trace is a free function over plain data: no stage object in the hot loop, and the same
// signature the GLSL twin implements. The material step functions live in
// Erosion_DropletTransfer_PROC.cpp; the lifecycle loop in Erosion_Droplet_PROC.cpp.
#pragma once
#include "Erosion_Column_PROC.h"

namespace SanmapGen {
namespace Proc {

struct DropletState {
    float positionX  = 0.0f, positionY  = 0.0f;
    float directionX = 0.0f, directionY = 0.0f;
    float speed = 1.0f, water = 1.0f, sediment = 0.0f;
};

// A view of the erosion state one pass mutates. Not owning; the stage supplies the storage.
struct DropletContext {
    int*                              thicknessFixedPoint = nullptr;
    const float*                      materialPhysics     = nullptr;
    const ErosionKernelConfiguration* configuration       = nullptr;
    const float*                      dropletSpawns       = nullptr;   // interleaved x,y
    int                               cellCount           = 0;
};

inline float ClampUnit(float value) { return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); }

// Erosion_DropletTransfer_PROC.cpp
float DepositSplat(const DropletContext& context, int nodeX, int nodeY, float fractionX, float fractionY,
                   float amountHeight);
float ErodeSplat(const DropletContext& context, int nodeX, int nodeY, float fractionX, float fractionY,
                 float amountHeight);
void  SteerDroplet(const ErosionKernelConfiguration& configuration, DropletState& state,
                   float gradientX, float gradientY, float friction, unsigned int stepSeed);
void  ExchangeSediment(const DropletContext& context, DropletState& state, float deltaHeight,
                       float capacity, float hardness, int nodeX, int nodeY,
                       float fractionX, float fractionY);
void  SettleDroplet(const DropletContext& context, float sediment, int nodeX, int nodeY);

// Erosion_Droplet_PROC.cpp
void TraceSingleDroplet(const DropletContext& context, int dropletIndex);

} // namespace Proc
} // namespace SanmapGen

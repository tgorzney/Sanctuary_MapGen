#version 430 core
// Erosion_Splat_PROC.glsl — the four-corner bilinear transfers, Gpu twin of the splat helpers
// in Erosion_DropletTransfer_PROC.cpp. Its own compilation unit so the droplet kernel stays
// inside the ARCH §1.5 file ceiling; it owns no buffers and takes plain scalars, calling the
// column primitives (which hold the atomics) by prototype.
// Every function reports the height it ACTUALLY moved, in fixed-point ticks converted back —
// that is what keeps the volume book balanced when a thin column runs out mid-carve.

int   heightToFixedPoint(float height, float fixedPointScale);
float fixedPointToHeight(int fixedTicks, float fixedPointInverse);
int   erodeColumnClamped(int cellCount, int cellIndex, int highestStratum, int requestedTicks);
int   depositColumn(int cellCount, int cellIndex, int stratum, int amountTicks);

// Corner order matches the Cpu footprint exactly: (x,y), (x+1,y), (x,y+1), (x+1,y+1).
int cornerCellIndex(int vertexSize, int nodeX, int nodeY, int corner) {
    return (nodeY + (corner >> 1)) * vertexSize + nodeX + (corner & 1);
}
float cornerWeight(float fractionX, float fractionY, int corner) {
    float weightX = (corner & 1) == 0 ? 1.0 - fractionX : fractionX;
    float weightY = (corner >> 1) == 0 ? 1.0 - fractionY : fractionY;
    return weightX * weightY;
}

float depositSplat(int cellCount, int vertexSize, int depositStratum, float fixedPointScale,
                   float fixedPointInverse, int nodeX, int nodeY, float fractionX, float fractionY,
                   float amountHeight) {
    int addedTicks = 0;
    for (int corner = 0; corner < 4; ++corner)
        addedTicks += depositColumn(cellCount, cornerCellIndex(vertexSize, nodeX, nodeY, corner), depositStratum,
                                    heightToFixedPoint(amountHeight * cornerWeight(fractionX, fractionY, corner),
                                                       fixedPointScale));
    return fixedPointToHeight(addedTicks, fixedPointInverse);
}

float erodeSplat(int cellCount, int vertexSize, int highestErodableStratum, float fixedPointScale,
                 float fixedPointInverse, int nodeX, int nodeY, float fractionX, float fractionY,
                 float amountHeight) {
    int removedTicks = 0;
    for (int corner = 0; corner < 4; ++corner)
        removedTicks += erodeColumnClamped(cellCount, cornerCellIndex(vertexSize, nodeX, nodeY, corner),
                                           highestErodableStratum,
                                           heightToFixedPoint(amountHeight * cornerWeight(fractionX, fractionY, corner),
                                                              fixedPointScale));
    return fixedPointToHeight(removedTicks, fixedPointInverse);
}

// Dump whatever the droplet still carries at its last valid cell, so eroded volume returns to
// the map instead of vanishing (the conservation sanity the acceptance test checks).
void settleDroplet(int cellCount, int vertexSize, int depositStratum, int bConserveSedimentAtExit,
                   float sedimentMinimum, float fixedPointScale, float sediment, int nodeX, int nodeY) {
    if (bConserveSedimentAtExit == 0 || sediment <= sedimentMinimum) return;
    int cellIndex = nodeY * vertexSize + nodeX;
    if (cellIndex < 0 || cellIndex >= cellCount) return;
    depositColumn(cellCount, cellIndex, depositStratum, heightToFixedPoint(sediment, fixedPointScale));
}

#include "Gen_Placement.h"
#include "../TerrainGenerator.h"
#include "Gen_Mask_Slope.h"
#include "Gen_Marker_Procedural.h"

namespace SanmapGen {

void Gen_Placement::Process(const FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult, size_t currentErosionHash, size_t currentFlowHash) {
        int vertSize = params.MapSize + 1;
size_t currentPlacementHash = params.GetPlacementHash(currentFlowHash);
        bool skipPlacement = false;
        
        if (currentPlacementHash == inOutResult.CachedPlacementHash) {
            skipPlacement = true;
            // GeneratedMarkers is already cached in inOutResult!
        }
        
        if (!skipPlacement) {
            if (params.FastPreviewMode) return;
            // 1. Calculate slopemap for procedural rules (Cached)
            if (inOutResult.CachedSlopeHash != currentErosionHash || inOutResult.CachedSlopeMap.GetWidth() != vertSize) {
                inOutResult.CachedSlopeMap.Resize(vertSize, vertSize, 0.0f);
                Gen_Mask_Slope::GenerateSlopeMap(outMap, inOutResult.CachedSlopeMap, params.SlopeSettingsParams.bUseEngineParityMath, &inOutResult, params.TerrainMaxHeight);
                inOutResult.CachedSlopeHash = currentErosionHash;
            }
            
            // 2. Generate Procedural Markers
            inOutResult.GeneratedMarkers.clear();
            if (params.EnableProceduralMarkers) {
                Gen_Marker_Procedural::GenerateProceduralMarkers(params, outMap, inOutResult.CachedSlopeMap, inOutResult);
            }
            
            inOutResult.CachedPlacementHash = currentPlacementHash;
        }
    
    }



} // namespace SanmapGen

#include "Gen_Tex_Albedo.h"
#include <algorithm>

namespace SanmapGen {
    void Gen_Tex_Albedo::ApplyAlbedoMask(std::vector<FloatMask>& materialMasks, float mask,
                                         const NoiseLayer& layer, const GenerationParams& params, int x, int y) {
        float finalMask = mask * layer.Opacity;
        
        if (layer.StratumIndex < params.Stratums.size() && params.Stratums[layer.StratumIndex].UseImportedMask && !params.Stratums[layer.StratumIndex].ImportedMaskData.empty()) {
            int texSize = params.MapSize;
            int sx = std::min(x, texSize - 1);
            int sy = std::min(y, texSize - 1);
            finalMask = params.Stratums[layer.StratumIndex].ImportedMaskData[sy * texSize + sx];
        }
        
        int sIdx = std::clamp(layer.StratumIndex, 0, 8);
        float currentMask = materialMasks[sIdx].Get(x, y);
        materialMasks[sIdx].Set(x, y, std::clamp(currentMask + finalMask, 0.0f, 1.0f));
    }
}

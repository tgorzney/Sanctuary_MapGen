import os

with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# SymmetrizeErodedTerrain bounds
sym_start = 43
sym_end = 110
# ProcessErosion bounds
proc_start = 137
proc_end = 489

sym_body = ''.join(lines[sym_start:sym_end+1])
sym_body = sym_body.replace('FloatMask TerrainGenerator::SymmetrizeErodedTerrain', 'FloatMask Gen_Erosion::SymmetrizeErodedTerrain')

proc_body = ''.join(lines[proc_start:proc_end+1])
proc_body = proc_body.replace('void TerrainGenerator::ProcessErosion', 'void Gen_Erosion::Process')

# Write Gen_Erosion.h
h_content = '''#pragma once
#include <vector>
#include "../Parameters.h"
#include "../Mask2D.h"

namespace SanmapGen {
    struct GenerationResult;

    class Gen_Erosion {
    public:
        static void Process(FloatMask& outMap, std::vector<FloatMask>& Stratums, const GenerationParams& params, GenerationResult& inOutResult, size_t currentBlendHash, size_t& outErosionHash);
        
    private:
        static FloatMask SymmetrizeErodedTerrain(const FloatMask& terrainMap, const NoiseLayer& layer, const GenerationParams& params);
    };
}
'''
with open('core/gen/Gen_Erosion.h', 'w', encoding='utf-8') as f:
    f.write(h_content)

# Write Gen_Erosion.cpp
cpp_content = '''#include "Gen_Erosion.h"
#include "../TerrainGenerator.h"
#include "Gen_Noise.h"
#include "Gen_Mask_Height.h"
#include "../TerrainCompute.h"
#include "../ErosionCompute.h"
#include "../ErosionSimulator.h"
#include "../MathUtils.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace SanmapGen {

''' + sym_body + '''

''' + proc_body + '''

} // namespace SanmapGen
'''
with open('core/gen/Gen_Erosion.cpp', 'w', encoding='utf-8') as f:
    f.write(cpp_content)

# Remove from TerrainGenerator.cpp
new_lines = []
for i in range(len(lines)):
    if sym_start <= i <= sym_end:
        continue
    if proc_start <= i <= proc_end:
        continue
    new_lines.append(lines[i])

new_content = ''.join(new_lines)
new_content = new_content.replace('ProcessErosion(', 'Gen_Erosion::Process(')

# Add #include "gen/Gen_Erosion.h" to TerrainGenerator.cpp
new_content = '#include "gen/Gen_Erosion.h"\n' + new_content

with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(new_content)

# Remove from TerrainGenerator.h
with open('core/TerrainGenerator.h', 'r', encoding='utf-8') as f:
    h_lines = f.readlines()

with open('core/TerrainGenerator.h', 'w', encoding='utf-8') as f:
    for line in h_lines:
        if 'ProcessErosion' not in line and 'SymmetrizeErodedTerrain' not in line:
            f.write(line)

print("Step 1.2 Complete")

import os

with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

start_idx = 135
end_idx = 290

func_body = ''.join(lines[start_idx:end_idx+1])

# Replace TerrainGenerator::ProcessNoiseAndBlend with Gen_NoiseAndBlend::Process
func_body = func_body.replace('void TerrainGenerator::ProcessNoiseAndBlend', 'void Gen_NoiseAndBlend::Process')

# Write Gen_NoiseAndBlend.h
h_content = '''#pragma once
#include <vector>
#include "../Parameters.h"
#include "../Mask2D.h"

namespace SanmapGen {
    class Gen_NoiseAndBlend {
    public:
        static void Process(FloatMask& outMap, std::vector<FloatMask>& Stratums, const GenerationParams& params, GenerationResult& inOutResult, size_t& outBlendHash);
    };
}
'''
with open('core/gen/Gen_NoiseAndBlend.h', 'w', encoding='utf-8') as f:
    f.write(h_content)

# Write Gen_NoiseAndBlend.cpp
cpp_content = '''#include "Gen_NoiseAndBlend.h"
#include "../TerrainCompute.h"
#include "Gen_Noise.h"
#include "Gen_Mask_Height.h"
#include <algorithm>

namespace SanmapGen {

''' + func_body + '''

} // namespace SanmapGen
'''
with open('core/gen/Gen_NoiseAndBlend.cpp', 'w', encoding='utf-8') as f:
    f.write(cpp_content)

# Remove from TerrainGenerator.cpp
new_lines = lines[:start_idx] + lines[end_idx+1:]
new_content = ''.join(new_lines)
new_content = new_content.replace('ProcessNoiseAndBlend(', 'Gen_NoiseAndBlend::Process(')

# Add #include "gen/Gen_NoiseAndBlend.h" to TerrainGenerator.cpp
new_content = '#include "gen/Gen_NoiseAndBlend.h"\\n' + new_content

with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(new_content)

# Remove from TerrainGenerator.h
with open('core/TerrainGenerator.h', 'r', encoding='utf-8') as f:
    h_lines = f.readlines()

with open('core/TerrainGenerator.h', 'w', encoding='utf-8') as f:
    for line in h_lines:
        if 'ProcessNoiseAndBlend' not in line:
            f.write(line)

print("Step 1.1 Complete")

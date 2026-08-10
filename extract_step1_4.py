import os

with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

proc_start = 73
proc_end = 102

proc_body = ''.join(lines[proc_start:proc_end+1])
proc_body = proc_body.replace('void TerrainGenerator::ProcessPlacement', 'void Gen_Placement::Process')

# Write Gen_Placement.h
h_content = '''#pragma once
#include <vector>
#include "../Parameters.h"
#include "../Mask2D.h"

namespace SanmapGen {
    struct GenerationResult;

    class Gen_Placement {
    public:
        static void Process(const FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult, size_t currentErosionHash, size_t currentFlowHash);
    };
}
'''
with open('core/gen/Gen_Placement.h', 'w', encoding='utf-8') as f:
    f.write(h_content)

# Write Gen_Placement.cpp
cpp_content = '''#include "Gen_Placement.h"
#include "../TerrainGenerator.h"
#include "Gen_Mask_Slope.h"
#include "Gen_Marker_Procedural.h"

namespace SanmapGen {

''' + proc_body + '''

} // namespace SanmapGen
'''
with open('core/gen/Gen_Placement.cpp', 'w', encoding='utf-8') as f:
    f.write(cpp_content)

# Remove from TerrainGenerator.cpp
new_lines = []
for i in range(len(lines)):
    if proc_start <= i <= proc_end:
        continue
    new_lines.append(lines[i])

new_content = ''.join(new_lines)
new_content = new_content.replace('ProcessPlacement(', 'Gen_Placement::Process(')

# Add #include "gen/Gen_Placement.h" to TerrainGenerator.cpp
new_content = '#include "gen/Gen_Placement.h"\n' + new_content

with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(new_content)

# Remove from TerrainGenerator.h
with open('core/TerrainGenerator.h', 'r', encoding='utf-8') as f:
    h_lines = f.readlines()

with open('core/TerrainGenerator.h', 'w', encoding='utf-8') as f:
    for line in h_lines:
        if 'ProcessPlacement' not in line:
            f.write(line)

print("Step 1.4 and 1.5 Complete")

import os

with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

proc_start = 71
proc_end = 252

proc_body = ''.join(lines[proc_start:proc_end+1])
proc_body = proc_body.replace('void TerrainGenerator::ProcessFlow', 'void Gen_FlowAndAccumulation::Process')

# Apply math fix
proc_body = proc_body.replace('flowPtr[bestY*vertSize + bestX] += transfer + maxDrop;', 
    'float incomingVel = flowPtr[y*vertSize + px];\n'
    '                                                float newVel = (incomingVel + maxDrop) * 0.85f;\n'
    '                                                #pragma omp atomic\n'
    '                                                flowPtr[bestY*vertSize + bestX] += newVel;')

proc_body = proc_body.replace('flowPtr[by*vertSize + bx] += transfer + maxDropArr[i];', 
    'float incomingVel = flowPtr[y*vertSize + x + i];\n'
    '                                            float newVel = (incomingVel + maxDropArr[i]) * 0.85f;\n'
    '                                            #pragma omp atomic\n'
    '                                            flowPtr[by*vertSize + bx] += newVel;')

# Write Gen_FlowAndAccumulation.h
h_content = '''#pragma once
#include <vector>
#include "../Parameters.h"
#include "../Mask2D.h"

namespace SanmapGen {
    struct GenerationResult;

    class Gen_FlowAndAccumulation {
    public:
        static void Process(const FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult, size_t currentErosionHash, size_t& outFlowHash);
    };
}
'''
with open('core/gen/Gen_FlowAndAccumulation.h', 'w', encoding='utf-8') as f:
    f.write(h_content)

# Write Gen_FlowAndAccumulation.cpp
cpp_content = '''#include "Gen_FlowAndAccumulation.h"
#include "../TerrainGenerator.h"
#include "../math/Sanmath_SIMD.h"
#include <algorithm>
#include <cmath>

namespace SanmapGen {

''' + proc_body + '''

} // namespace SanmapGen
'''
with open('core/gen/Gen_FlowAndAccumulation.cpp', 'w', encoding='utf-8') as f:
    f.write(cpp_content)

# Remove from TerrainGenerator.cpp
new_lines = []
for i in range(len(lines)):
    if proc_start <= i <= proc_end:
        continue
    new_lines.append(lines[i])

new_content = ''.join(new_lines)
new_content = new_content.replace('ProcessFlow(', 'Gen_FlowAndAccumulation::Process(')

# Add #include "gen/Gen_FlowAndAccumulation.h" to TerrainGenerator.cpp
new_content = '#include "gen/Gen_FlowAndAccumulation.h"\n' + new_content

with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(new_content)

# Remove from TerrainGenerator.h
with open('core/TerrainGenerator.h', 'r', encoding='utf-8') as f:
    h_lines = f.readlines()

with open('core/TerrainGenerator.h', 'w', encoding='utf-8') as f:
    for line in h_lines:
        if 'ProcessFlow' not in line:
            f.write(line)

print("Step 1.3 Complete")

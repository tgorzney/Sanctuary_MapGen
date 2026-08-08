import os

with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Remove EncodeMorton2D and DecodeMorton2D from TerrainGenerator.cpp
start = content.find('uint32_t TerrainGenerator::Gen_Noise::EncodeMorton2D')
if start != -1:
    end = content.find('FloatMask TerrainGenerator::SymmetrizeErodedTerrain')
    content = content[:start] + '\n\n' + content[end:]
    
content = content.replace('TerrainGenerator::Gen_Noise::', 'Gen_Noise::')

if '#include "gen/Gen_Marker_Procedural.h"' not in content:
    content = content.replace('#include "gen/Gen_Noise.h"', '#include "gen/Gen_Noise.h"\n#include "gen/Gen_Marker_Procedural.h"')

content = content.replace(' GenerateProceduralMarkers(', ' Gen_Marker_Procedural::GenerateProceduralMarkers(')

with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

with open('core/gen/Gen_Marker_Procedural.cpp', 'r', encoding='utf-8') as f:
    proc_content = f.read()
proc_content = proc_content.replace('void TerrainGenerator::GenerateProceduralMarkers', 'void Gen_Marker_Procedural::GenerateProceduralMarkers')
proc_content = proc_content.replace('#include "../Parameters.h"', '#include "../Parameters.h"\n#include "Gen_Marker_Procedural.h"')

with open('core/gen/Gen_Marker_Procedural.cpp', 'w', encoding='utf-8') as f:
    f.write(proc_content)

hdr = '''#pragma once
#include "../Parameters.h"
#include "../Mask2D.h"

namespace SanmapGen {
    class Gen_Marker_Procedural {
    public:
        static void GenerateProceduralMarkers(const GenerationParams& params, const FloatMask& heightmap, const FloatMask& slopeMap, GenerationResult& inOutResult);
    };
}
'''
with open('core/gen/Gen_Marker_Procedural.h', 'w', encoding='utf-8') as f:
    f.write(hdr)

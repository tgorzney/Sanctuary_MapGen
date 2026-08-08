import os

with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

start = content.find('void TerrainGenerator::GenerateProceduralMarkers')
if start != -1:
    end = content.find('void TerrainGenerator::CalculateMarkerSymmetryGroups', start)
    if end == -1: end = len(content)
    
    proc_content = content[start:end]
    
    with open('core/gen/Gen_Marker_Procedural.cpp', 'w', encoding='utf-8') as out:
        out.write('#include "../TerrainGenerator.h"\n')
        out.write('#include "../Parameters.h"\n')
        out.write('#include <random>\n')
        out.write('#include <omp.h>\n')
        out.write('#include <mutex>\n')
        out.write('#include <atomic>\n')
        out.write('#include <cmath>\n\n')
        out.write('namespace SanmapGen {\n\n')
        
        helper_start = content.find('struct MarkerCandidate')
        if helper_start != -1:
            helper_end = content.find('};', helper_start) + 2
            struct_str = content[helper_start:helper_end]
            out.write('    ' + struct_str.replace('\n', '\n    ') + '\n\n')
            
            # also remove struct from original file
            content = content[:helper_start] + content[helper_end:]
            # update start/end indices because content length changed
            start = content.find('void TerrainGenerator::GenerateProceduralMarkers')
            end = content.find('void TerrainGenerator::CalculateMarkerSymmetryGroups', start)
            proc_content = content[start:end]
            
        out.write(proc_content)
        out.write('} // namespace SanmapGen\n')
        
    print('Successfully extracted Gen_Marker_Procedural.cpp')
    
    new_content = content[:start] + content[end:]
    with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as out:
        out.write(new_content)

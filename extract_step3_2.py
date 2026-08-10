import os

with open('core/Parameters.h', 'r', encoding='utf-8') as f:
    lines = f.readlines()

def extract_struct(name, out_lines):
    start = -1
    for i, line in enumerate(lines):
        if f'struct {name} ' in line or f'struct {name}' in line:
            start = i
            break
    if start == -1: return
    brace_count = 0
    in_struct = False
    end = -1
    for i in range(start, len(lines)):
        if '{' in lines[i]:
            brace_count += lines[i].count('{')
            in_struct = True
        if '}' in lines[i]:
            brace_count -= lines[i].count('}')
        if in_struct and brace_count == 0:
            end = i
            if i+1 < len(lines) and ';' in lines[i+1]:
                end = i+1
            elif ';' in lines[i]:
                pass
            break
    if end != -1:
        out_lines.extend(lines[start:end+1])
        out_lines.append('\n')
        for i in range(start, end+1):
            lines[i] = ''

out_geom = []
extract_struct('Point2D', out_geom)
extract_struct('GradientStop', out_geom)
extract_struct('GradientSettings', out_geom)
extract_struct('SlopeSettings', out_geom)
extract_struct('NoiseLayer', out_geom)
extract_struct('StratumSettings', out_geom)
extract_struct('MarkerRule', out_geom)
extract_struct('PropRule', out_geom)
extract_struct('DecalRule', out_geom)
extract_struct('ProceduralMarkerLayer', out_geom)
extract_struct('PlacedMarkerLayer', out_geom)
extract_struct('GeoLayerDef', out_geom)

h_content = '''#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
''' + ''.join(out_geom) + '''
}
'''

with open('core/params/Params_Geometry.h', 'w', encoding='utf-8') as f:
    f.write(h_content)

new_content = ''.join(lines)
new_content = new_content.replace('#pragma once', '#pragma once\n#include "params/Params_Geometry.h"')

with open('core/Parameters.h', 'w', encoding='utf-8') as f:
    f.write(new_content)

print("Step 3.2 Extracted")

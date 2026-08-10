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

out_env = []
extract_struct('WaterSettings', out_env)
extract_struct('AtmosphereSettings', out_env)
extract_struct('GlobalTexturingSettings', out_env)
extract_struct('SanTextureLoader', out_env)
extract_struct('SanNormalTextureLoader', out_env)
extract_struct('SanMaskTextureLoader', out_env)
extract_struct('SanVector2', out_env)
extract_struct('SanVector4', out_env)
extract_struct('SanColor', out_env)
extract_struct('UnitTransform', out_env)
extract_struct('UnitGroup', out_env)
extract_struct('Army', out_env)
extract_struct('MarkerTransform', out_env)

h_content = '''#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
''' + ''.join(out_env) + '''
}
'''

with open('core/params/Params_Environment.h', 'w', encoding='utf-8') as f:
    f.write(h_content)

new_content = ''.join(lines)
new_content = new_content.replace('#pragma once', '#pragma once\n#include "params/Params_Environment.h"')

with open('core/Parameters.h', 'w', encoding='utf-8') as f:
    f.write(new_content)

print("Step 3.3 Extracted")

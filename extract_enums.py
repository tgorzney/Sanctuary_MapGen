import os

with open('core/Parameters.h', 'r', encoding='utf-8') as f:
    lines = f.readlines()

def extract_enum(name, out_lines):
    start = -1
    for i, line in enumerate(lines):
        if f'enum {name}' in line or f'enum class {name}' in line:
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

out_enums = []
extract_enum('SymmetryFlags', out_enums)
extract_enum('BlendMode', out_enums)
extract_enum('NoiseType', out_enums)
extract_enum('FractalType', out_enums)
extract_enum('ImportedMaskMode', out_enums)
extract_enum('LayerType', out_enums)

h_content = '''#pragma once

namespace SanmapGen {
''' + ''.join(out_enums) + '''
}
'''

with open('core/params/Params_Enums.h', 'w', encoding='utf-8') as f:
    f.write(h_content)

new_content = ''.join(lines)
new_content = new_content.replace('#include "params/Params_Environment.h"', '#include "params/Params_Enums.h"\n#include "params/Params_Environment.h"')

with open('core/Parameters.h', 'w', encoding='utf-8') as f:
    f.write(new_content)


import os

with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

def extract_function(name):
    # Try different signatures
    start = content.find(f'float TerrainGenerator::{name}')
    if start == -1:
        start = content.find(f'void TerrainGenerator::{name}')
    if start == -1: return ''
    
    end = content.find('}', start)
    depth = 0
    in_func = False
    for i in range(start, len(content)):
        if content[i] == '{':
            depth += 1
            in_func = True
        elif content[i] == '}':
            depth -= 1
            if in_func and depth == 0:
                end = i + 1
                break
                
    return content[start:end]

funcs = ['ProcessLayerChunk', 'EvaluateSymmetricNoise', 'BilinearGet', 'ApplySymmetryBlur']

out_content = '#include "Gen_Noise.h"\n#include "../TerrainGenerator.h"\n#include <cmath>\n#include <omp.h>\n\nnamespace SanmapGen {\n\n'

for f in funcs:
    s = extract_function(f)
    if s:
        # replace TerrainGenerator:: with Gen_Noise::
        s = s.replace(f'TerrainGenerator::{f}', f'Gen_Noise::{f}')
        out_content += s + '\n\n'
        content = content.replace(extract_function(f), '')
    else:
        print(f'Warning: {f} not found!')

out_content += '}\n'

with open('core/gen/Gen_Noise.cpp', 'w', encoding='utf-8') as f:
    f.write(out_content)
    
with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
    
print('Extracted Gen_Noise.cpp')

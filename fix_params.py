import os

with open('old_params.txt', 'r', encoding='utf-16') as f:
    content = f.read()

def extract_struct(name):
    start = content.find(f'struct {name} {{')
    if start == -1:
        start = content.find(f'struct {name}\n{{')
    if start == -1: return ''
    
    depth = 0
    in_struct = False
    for i in range(start, len(content)):
        if content[i] == '{':
            depth += 1
            in_struct = True
        elif content[i] == '}':
            depth -= 1
            if in_struct and depth == 0:
                semi = content.find(';', i)
                return content[start:semi+1]
    return ''

missing = ['ErosionSettings', 'PropDef', 'DecalDef', 'SplatLayer']
missing_text = ''
for m in missing:
    s = extract_struct(m)
    if s:
        missing_text += '    ' + s.replace('\n', '\n    ') + '\n\n'
    else:
        print(f'Warning: {m} not found!')

with open('core/Parameters.h', 'r', encoding='utf-8') as f:
    param_content = f.read()

if '#include "FastNoiseLite.h"' not in param_content:
    param_content = param_content.replace('// Std lib', '#include "FastNoiseLite.h"\n// Std lib')

idx = param_content.find('struct GenerationParams')
new_param_content = param_content[:idx] + missing_text + param_content[idx:]

with open('core/Parameters.h', 'w', encoding='utf-8') as f:
    f.write(new_param_content)

print('Restored additional missing structures.')

import os

with open('old_params.txt', 'r', encoding='utf-16') as f:
    content = f.read()

def extract_enum(name):
    start = content.find(f'enum class {name} {{')
    if start == -1: return ''
    end = content.find('};', start)
    return content[start:end+2]

missing = ['BlendMode', 'NoiseType', 'FractalType', 'GradientType', 'EdgeType']
missing_text = ''
for m in missing:
    s = extract_enum(m)
    if s:
        missing_text += '    ' + s.replace('\n', '\n    ') + '\n\n'
    else:
        print(f'Warning: {m} not found!')

with open('core/Parameters.h', 'r', encoding='utf-8') as f:
    param_content = f.read()

idx = param_content.find('struct GenerationParams')
new_param_content = param_content[:idx] + missing_text + param_content[idx:]

with open('core/Parameters.h', 'w', encoding='utf-8') as f:
    f.write(new_param_content)

print('Restored enum structures.')

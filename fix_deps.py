import os

# 1. Update Params_Enums.h with missing Enums
with open('core/params/Params_Enums.h', 'r', encoding='utf-8') as f:
    enums_content = f.read()

enums_to_add = '''
    enum class SymmetryAlgorithm {
        Fold,
        Blur,
        CrossFade,
        Cylinder3D,
        Torus3D,
        NativeHash,
        Superposition
    };

    enum MarkerPriority {
        Priority_LargestArea = 0,
        Priority_SmallestArea = 1,
        Priority_LeastVariance = 2
    };
    
    enum MarkerGradientType {
        Gradient_None = 0,
        Gradient_CenterFocus = 1,
        Gradient_EdgeFocus = 2,
        Gradient_Torus = 3
    };
'''

enums_content = enums_content.replace('}', enums_to_add + '\n}')
with open('core/params/Params_Enums.h', 'w', encoding='utf-8') as f:
    f.write(enums_content)

# Remove those enums from Parameters.h
with open('core/Parameters.h', 'r', encoding='utf-8') as f:
    param_lines = f.readlines()

new_param_lines = []
skip = False
for line in param_lines:
    if 'enum class SymmetryAlgorithm' in line or 'enum MarkerPriority' in line or 'enum MarkerGradientType' in line:
        skip = True
    if skip and '};' in line:
        skip = False
        continue
    if not skip:
        new_param_lines.append(line)

with open('core/Parameters.h', 'w', encoding='utf-8') as f:
    f.writelines(new_param_lines)

# 2. Extract Gradients into Params_Gradients.h
with open('core/params/Params_Geometry.h', 'r', encoding='utf-8') as f:
    geo_lines = f.readlines()

grad_lines = []
new_geo_lines = []
skip_geo = False
for line in geo_lines:
    if 'struct GradientStop {' in line or 'struct GradientSettings {' in line:
        skip_geo = True
    if skip_geo:
        grad_lines.append(line)
        if '};' in line:
            skip_geo = False
    else:
        new_geo_lines.append(line)

grad_content = '''#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
''' + ''.join(grad_lines) + '''
}
'''
with open('core/params/Params_Gradients.h', 'w', encoding='utf-8') as f:
    f.write(grad_content)

with open('core/params/Params_Geometry.h', 'w', encoding='utf-8') as f:
    content = ''.join(new_geo_lines)
    content = content.replace('#include "Params_Enums.h"', '#include "Params_Enums.h"\n#include "Params_Gradients.h"')
    f.write(content)

with open('core/params/Params_ErosionFlow.h', 'r', encoding='utf-8') as f:
    ef_content = f.read()
ef_content = ef_content.replace('#include "Params_Enums.h"', '#include "Params_Enums.h"\n#include "Params_Gradients.h"')
with open('core/params/Params_ErosionFlow.h', 'w', encoding='utf-8') as f:
    f.write(ef_content)

print("Fixed circular dependencies and enums")

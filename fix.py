# 1. Fix TerrainGenerator.cpp line 1
with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
    tg_content = f.read()
tg_content = tg_content.replace('#include "gen/Gen_NoiseAndBlend.h"\\n', '#include "gen/Gen_NoiseAndBlend.h"\n')
with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(tg_content)

# 2. Add GenerationResult forward decl to Gen_NoiseAndBlend.h
with open('core/gen/Gen_NoiseAndBlend.h', 'r', encoding='utf-8') as f:
    h_content = f.read()
h_content = h_content.replace('namespace SanmapGen {', 'namespace SanmapGen {\n    struct GenerationResult;\n')
with open('core/gen/Gen_NoiseAndBlend.h', 'w', encoding='utf-8') as f:
    f.write(h_content)

# 3. Add #include "../TerrainGenerator.h" to Gen_NoiseAndBlend.cpp
with open('core/gen/Gen_NoiseAndBlend.cpp', 'r', encoding='utf-8') as f:
    cpp_content = f.read()
cpp_content = cpp_content.replace('#include "Gen_NoiseAndBlend.h"', '#include "Gen_NoiseAndBlend.h"\n#include "../TerrainGenerator.h"')
with open('core/gen/Gen_NoiseAndBlend.cpp', 'w', encoding='utf-8') as f:
    f.write(cpp_content)

# 4. Add GenerationResult forward decl to Gen_Marker_Procedural.h
with open('core/gen/Gen_Marker_Procedural.h', 'r', encoding='utf-8') as f:
    mp_h = f.read()
if 'struct GenerationResult;' not in mp_h:
    mp_h = mp_h.replace('namespace SanmapGen {', 'namespace SanmapGen {\n    struct GenerationResult;\n')
    with open('core/gen/Gen_Marker_Procedural.h', 'w', encoding='utf-8') as f:
        f.write(mp_h)

print('Fixed!')

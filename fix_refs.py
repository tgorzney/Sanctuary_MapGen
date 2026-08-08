import os

with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Add include
if '#include "gen/Gen_Noise.h"' not in content:
    content = content.replace('#include "TerrainGenerator.h"', '#include "TerrainGenerator.h"\n#include "gen/Gen_Noise.h"')

# Update function calls
content = content.replace('ProcessLayerChunk', 'Gen_Noise::ProcessLayerChunk')
content = content.replace('EvaluateSymmetricNoise', 'Gen_Noise::EvaluateSymmetricNoise')
content = content.replace('BilinearGet', 'Gen_Noise::BilinearGet')
content = content.replace('ApplySymmetryBlur', 'Gen_Noise::ApplySymmetryBlur')
content = content.replace('EncodeMorton2D', 'Gen_Noise::EncodeMorton2D')
content = content.replace('DecodeMorton2D', 'Gen_Noise::DecodeMorton2D')

with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print('Updated references in TerrainGenerator.cpp')

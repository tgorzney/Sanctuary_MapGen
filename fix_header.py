import os

with open('core/TerrainGenerator.h', 'r', encoding='utf-8') as f:
    content = f.read()

funcs = [
    'static inline uint32_t EncodeMorton2D(uint32_t x, uint32_t y);',
    'static inline void DecodeMorton2D(uint32_t code, uint32_t& x, uint32_t& y);',
    'static void ProcessLayerChunk(ChunkTask task);',
    'static float EvaluateSymmetricNoise(int px, int py, int mapSize, FastNoiseLite& noise, const NoiseLayer& layer, const GenerationParams* params);',
    'static float BilinearGet(const FloatMask& map, float x, float y);',
    'static void ApplySymmetryBlur(FloatMask& map, int mapSize, float blurRadius, int symmetryMask, int spawnPointCount);',
    'static void GenerateProceduralMarkers(const GenerationParams& params, const FloatMask& heightmap, const FloatMask& slopeMap, GenerationResult& inOutResult);'
]

for func in funcs:
    content = content.replace(func, '')

# also remove ChunkTask
struct_start = content.find('struct ChunkTask')
if struct_start != -1:
    struct_end = content.find('};', struct_start) + 2
    content = content[:struct_start] + content[struct_end:]

with open('core/TerrainGenerator.h', 'w', encoding='utf-8') as f:
    f.write(content)

print('Updated TerrainGenerator.h')

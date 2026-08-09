import sys

def main():
    with open('core/TerrainGenerator.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # Find the start of GenerateMap
    start_idx = content.find('void TerrainGenerator::GenerateMap(FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult) {')
    if start_idx == -1:
        print("Could not find GenerateMap")
        return

    # Find the end of GenerateMap by counting braces
    brace_count = 0
    in_function = False
    end_idx = -1
    for i in range(start_idx, len(content)):
        if content[i] == '{':
            brace_count += 1
            in_function = True
        elif content[i] == '}':
            brace_count -= 1
        
        if in_function and brace_count == 0:
            end_idx = i + 1
            break

    if end_idx == -1:
        print("Could not find end of GenerateMap")
        return

    old_func = content[start_idx:end_idx]

    # Split old_func into the 4 phases
    
    # 1. Setup & Blending
    blend_start = old_func.find('int vertSize = params.MapSize + 1;')
    erosion_start = old_func.find('// --- Process Erosion Sequentially Layer-by-Layer ---')
    flow_start = old_func.find('size_t currentFlowHash = params.GetFlowHash(currentErosionHash);')
    placement_start = old_func.find('size_t currentPlacementHash = params.GetPlacementHash(currentFlowHash);')
    
    blend_code = old_func[blend_start:erosion_start]
    erosion_code = old_func[erosion_start:flow_start]
    flow_code = old_func[flow_start:placement_start]
    placement_code = old_func[placement_start:len(old_func)-1] # exclude the last brace
    
    # Generate new GenerateMap
    new_generate_map = '''void TerrainGenerator::GenerateMap(FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult) {
        int vertSize = params.MapSize + 1;
        outMap.Resize(vertSize, vertSize, 0.0f);
        
        std::vector<FloatMask> Stratums;
        auto flatLayers = params.GetFlatLayers();
        for (size_t i = 0; i < flatLayers.size(); ++i) {
            Stratums.push_back(FloatMask(vertSize, vertSize, 0.0f));
        }
        
        size_t currentBlendHash = 0;
        ProcessNoiseAndBlend(outMap, Stratums, params, inOutResult, currentBlendHash);
        
        size_t currentErosionHash = 0;
        ProcessErosion(outMap, Stratums, params, inOutResult, currentBlendHash, currentErosionHash);
        
        size_t currentFlowHash = 0;
        ProcessFlow(outMap, params, inOutResult, currentErosionHash, currentFlowHash);
        
        ProcessPlacement(outMap, params, inOutResult, currentErosionHash, currentFlowHash);
    }'''

    # Generate ProcessNoiseAndBlend
    process_blend = '''void TerrainGenerator::ProcessNoiseAndBlend(FloatMask& outMap, std::vector<FloatMask>& Stratums, const GenerationParams& params, GenerationResult& inOutResult, size_t& outBlendHash) {
''' + blend_code + '''
        outBlendHash = currentBlendHash;
    }'''

    # Generate ProcessErosion
    process_erosion = '''void TerrainGenerator::ProcessErosion(FloatMask& outMap, std::vector<FloatMask>& Stratums, const GenerationParams& params, GenerationResult& inOutResult, size_t currentBlendHash, size_t& outErosionHash) {
        int vertSize = params.MapSize + 1;
        auto flatLayers = params.GetFlatLayers();
''' + erosion_code + '''
        outErosionHash = currentErosionHash;
    }'''

    # Generate ProcessFlow
    process_flow = '''void TerrainGenerator::ProcessFlow(const FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult, size_t currentErosionHash, size_t& outFlowHash) {
        int vertSize = params.MapSize + 1;
''' + flow_code + '''
        outFlowHash = currentFlowHash;
    }'''
    
    # Fix fast preview check for Placement
    placement_code = placement_code.replace('if (!skipPlacement) {', 'if (!skipPlacement) {\\n            if (params.FastPreviewMode) return;')

    # Generate ProcessPlacement
    process_placement = '''void TerrainGenerator::ProcessPlacement(const FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult, size_t currentErosionHash, size_t currentFlowHash) {
        int vertSize = params.MapSize + 1;
''' + placement_code + '''
    }'''

    new_content = content[:start_idx] + new_generate_map + '\\n\\n' + process_blend + '\\n\\n' + process_erosion + '\\n\\n' + process_flow + '\\n\\n' + process_placement + '\\n\\n' + content[end_idx:]

    with open('core/TerrainGenerator.cpp', 'w', encoding='utf-8') as f:
        f.write(new_content)

    print("Refactor complete")

if __name__ == '__main__':
    main()

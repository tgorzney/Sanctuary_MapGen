import re

def fix():
    with open(r'D:\Projects\Sanctuary\Map Generator\Sanctuary_MapGen-master (Broken)\gui\main.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # Fix UseGPU error
    content = content.replace('ImGui::Checkbox("Use GPU Compute##egpu", &layer.Erosion.UseGPU);', '')

    # Fix SliderFloat with 3 args
    content = re.sub(r'ImGui::SliderFloat\(([^,]+),\s*([^,]+),\s*([^,)]+)\)', r'ImGui::SliderFloat(\1, \2, \3, \3 + 10.0f)', content)

    # Output to intermediate file
    with open(r'D:\Projects\Sanctuary\Map Generator\gui\main_fixed.cpp', 'w', encoding='utf-8') as f:
        f.write(content)

fix()

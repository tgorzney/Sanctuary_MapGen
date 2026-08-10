import os

with open('gui/EnvironmentTabs.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

proc_start = 14
proc_end = 420

proc_body = ''.join(lines[proc_start:proc_end+1])

# Write Tab_Markers.cpp
cpp_content = '''#include "../UITabs.h"
#include "../UIHelpers.h"
#include "imgui.h"
#include "../FileDialog.h"
#include <GLFW/glfw3.h>

extern GLuint GetMarkerIcon(const std::string& typeName, SanmapGen::GenerationParams& params, void* openZipArchive = nullptr);
extern void ForceScanIcons(SanmapGen::GenerationParams& params);

namespace SanmapGen {
namespace UI {

''' + proc_body + '''

} // namespace UI
} // namespace SanmapGen
'''
with open('gui/tabs/Tab_Markers.cpp', 'w', encoding='utf-8') as f:
    f.write(cpp_content)

# Remove from EnvironmentTabs.cpp
new_lines = []
for i in range(len(lines)):
    if proc_start <= i <= proc_end:
        continue
    new_lines.append(lines[i])

new_content = ''.join(new_lines)

with open('gui/EnvironmentTabs.cpp', 'w', encoding='utf-8') as f:
    f.write(new_content)

print("Step 2.3 Complete")

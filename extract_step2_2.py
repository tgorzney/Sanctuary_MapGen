import os

with open('gui/EnvironmentTabs.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

proc_start = 13
proc_end = 50

proc_body = ''.join(lines[proc_start:proc_end+1])

# Write Tab_Atmosphere.cpp
cpp_content = '''#include "../UITabs.h"
#include "../UIHelpers.h"
#include "imgui.h"

namespace SanmapGen {
namespace UI {

''' + proc_body + '''

} // namespace UI
} // namespace SanmapGen
'''
with open('gui/tabs/Tab_Atmosphere.cpp', 'w', encoding='utf-8') as f:
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

print("Step 2.2 Complete")

import os

with open('gui/EnvironmentTabs.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

proc_start = 12
proc_end = 40

proc_body = ''.join(lines[proc_start:proc_end+1])

# Ensure the tabs/ folder exists
os.makedirs('gui/tabs', exist_ok=True)

# Write Tab_Water.cpp
cpp_content = '''#include "../UITabs.h"
#include "../UIHelpers.h"
#include "imgui.h"

namespace SanmapGen {
namespace UI {

''' + proc_body + '''

} // namespace UI
} // namespace SanmapGen
'''
with open('gui/tabs/Tab_Water.cpp', 'w', encoding='utf-8') as f:
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

print("Step 2.1 Complete")

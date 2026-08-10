import os

with open('gui/EnvironmentTabs.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

def get_bounds(start_idx):
    brace_count = 0
    in_func = False
    for i in range(start_idx, len(lines)):
        line = lines[i]
        if '{' in line:
            brace_count += line.count('{')
            in_func = True
        if '}' in line:
            brace_count -= line.count('}')
        if in_func and brace_count == 0:
            return i
    return -1

props_start = -1
armies_start = -1
for i, line in enumerate(lines):
    if 'void RenderPropsTab' in line:
        props_start = i
    if 'void RenderArmiesTab' in line:
        armies_start = i

props_end = get_bounds(props_start)
armies_end = get_bounds(armies_start)

props_body = ''.join(lines[props_start:props_end+1])
armies_body = ''.join(lines[armies_start:armies_end+1])

# Write Tab_Props.cpp
cpp_props = '''#include "../UITabs.h"
#include "../UIHelpers.h"
#include "imgui.h"

namespace SanmapGen {
namespace UI {

''' + props_body + '''

} // namespace UI
} // namespace SanmapGen
'''
with open('gui/tabs/Tab_Props.cpp', 'w', encoding='utf-8') as f:
    f.write(cpp_props)

# Write Tab_Armies.cpp
cpp_armies = '''#include "../UITabs.h"
#include "../UIHelpers.h"
#include "imgui.h"
#include <string>

namespace SanmapGen {
namespace UI {

''' + armies_body + '''

} // namespace UI
} // namespace SanmapGen
'''
with open('gui/tabs/Tab_Armies.cpp', 'w', encoding='utf-8') as f:
    f.write(cpp_armies)

# Delete EnvironmentTabs.cpp
os.remove('gui/EnvironmentTabs.cpp')

print("Step 2.4 Complete")

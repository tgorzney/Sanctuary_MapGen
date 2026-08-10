def prepend(file, includes):
    with open(file, 'r', encoding='utf-8') as f:
        content = f.read()
    content = content.replace('#pragma once', '#pragma once\n' + includes)
    with open(file, 'w', encoding='utf-8') as f:
        f.write(content)

prepend('core/params/Params_ErosionFlow.h', '#include "Params_Enums.h"\n#include <string>\n#include <vector>\n#include <map>\n')
prepend('core/params/Params_Geometry.h', '#include "Params_Enums.h"\n#include "Params_ErosionFlow.h"\n#include <string>\n#include <vector>\n#include <map>\n')
prepend('core/params/Params_Environment.h', '#include "Params_Enums.h"\n#include <string>\n#include <vector>\n#include <map>\n')


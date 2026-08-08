import os
import glob

for filepath in glob.glob('core/data/*.h'):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Fix the bad include that contains \n literally inside the quotes
    if '#include "MarkerType_Rule.h\\n"' in content:
        content = content.replace('#include "MarkerType_Rule.h\\n"', '#include "MarkerType_Rule.h"')
        
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
        
print('Fixed includes in headers')

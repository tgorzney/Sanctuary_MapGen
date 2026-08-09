import os
import re

for root, dirs, files in os.walk('.'):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.h'):
            path = os.path.join(root, f)
            try:
                with open(path, 'r', encoding='utf-8', errors='ignore') as file:
                    content = file.read()
                
                new_content = content
                
                # Fix Stratums back to .albedo.path
                new_content = re.sub(r'(Stratums\[[^\]]+\]|\bstratum|\bs)\.AlbedoPath', r'\1.albedo.path', new_content)
                new_content = re.sub(r'(Stratums\[[^\]]+\]|\bstratum|\bs)\.NormalPath', r'\1.normal.path', new_content)
                new_content = re.sub(r'(Stratums\[[^\]]+\]|\bstratum|\bs)\.MaskPath', r'\1.mask.path', new_content)
                
                # Also fix the imgui pointer accesses in MaterialTabs.cpp
                new_content = new_content.replace('albedo.path.c_str()', 'albedo.path.c_str()')
                
                if new_content != content:
                    with open(path, 'w', encoding='utf-8') as file:
                        file.write(new_content)
                    print(f"Fixed {path}")
            except Exception as e:
                pass

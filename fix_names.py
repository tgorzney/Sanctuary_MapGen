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
                
                # Fix Name -> name on Stratums
                new_content = re.sub(r'(Stratums\[[^\]]+\]|\bstratum|\bs)\.Name', r'\1.name', new_content)
                new_content = re.sub(r'->Name', r'->name', new_content)
                
                # Fix DecalRule accidentally replaced
                new_content = new_content.replace('.albedo.path', '.AlbedoPath')
                new_content = new_content.replace('.normal.path', '.NormalPath')
                
                # However, StratumSettings SHOULD have .albedo.path, so we need to put it back for StratumSettings
                # We can just change Decal access to AlbedoPath
                new_content = new_content.replace('Decals[i].AlbedoPath', 'Decals[i].AlbedoPath') # Wait, replace above made it AlbedoPath
                
                if new_content != content:
                    with open(path, 'w', encoding='utf-8') as file:
                        file.write(new_content)
                    print(f"Fixed {path}")
            except Exception as e:
                pass

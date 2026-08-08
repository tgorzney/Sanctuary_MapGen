import json
import os
import sys

path = r'C:\Users\Tylre Gorzney\.gemini\antigravity\brain'
for cid in ['0469c144-23c5-4280-9921-07d2b4449c2b', 'db4ed8df-1011-4abb-ac6b-4e7e2a5d9e80']:
    log_path = os.path.join(path, cid, '.system_generated', 'logs', 'transcript_full.jsonl')
    if os.path.exists(log_path):
        with open(log_path, 'r', encoding='utf-8') as f:
            for line in f:
                try:
                    data = json.loads(line)
                    content = data.get('content', '') or str(data)
                    if 'ImGui::Text("Marker Scale");' in content and 'params.Markers.erase' in content:
                        print('Found in', cid, 'step', data.get('step_index'))
                        with open('found_ui.txt', 'w', encoding='utf-8') as out:
                            out.write(content)
                        sys.exit(0)
                except Exception as e:
                    pass

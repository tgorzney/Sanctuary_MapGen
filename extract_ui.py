import re
with open('restored_ui.txt', 'r', encoding='utf-8') as f:
    text = f.read()
    match = re.search(r'(ImGui::Text\("Procedural Marker Generation.*?)ImGui::Text\("Placed Markers', text, re.DOTALL)
    if match:
        with open('extracted_ui.txt', 'w', encoding='utf-8') as out:
            out.write(match.group(1))
        print("Success")
    else:
        print("Not found")

import os

with open("core/Parameters.h", "r", encoding="utf-8") as f:
    content = f.read()

# For ProceduralMarkerLayer
if "bool Enabled = true;\n        std::vector<MarkerRule> Rules;" in content:
    content = content.replace("bool Enabled = true;\n        std::vector<MarkerRule> Rules;", "bool Enabled = true;\n        bool Locked = false;\n        std::vector<MarkerRule> Rules;")
elif "bool Enabled = true;\r\n        std::vector<MarkerRule> Rules;" in content:
    content = content.replace("bool Enabled = true;\r\n        std::vector<MarkerRule> Rules;", "bool Enabled = true;\r\n        bool Locked = false;\r\n        std::vector<MarkerRule> Rules;")
    
# For PlacedMarkerLayer
if "bool Enabled = true;\n        std::vector<std::string> MarkerKeys;" in content:
    content = content.replace("bool Enabled = true;\n        std::vector<std::string> MarkerKeys;", "bool Enabled = true;\n        bool Locked = false;\n        std::vector<std::string> MarkerKeys;")
elif "bool Enabled = true;\r\n        std::vector<std::string> MarkerKeys;" in content:
    content = content.replace("bool Enabled = true;\r\n        std::vector<std::string> MarkerKeys;", "bool Enabled = true;\r\n        bool Locked = false;\r\n        std::vector<std::string> MarkerKeys;")

with open("core/Parameters.h", "w", encoding="utf-8") as f:
    f.write(content)

import os

with open("gui/UIHelpers.h", "r", encoding="utf-8") as f:
    content = f.read()

# Update signature
old_sig = """    std::vector<T>& layers, 
    std::function<void(T& layer, size_t index, bool& bNeedsUpdate, bool& bDeleteLayer)> renderLayerContent,
    bool& bNeedsMapUpdate,
    int* selectedIndex = nullptr)
{"""

new_sig = """    std::vector<T>& layers, 
    std::function<void(T& layer, size_t index, bool& bNeedsUpdate, bool& bDeleteLayer)> renderLayerContent,
    bool& bNeedsMapUpdate,
    int* selectedIndex = nullptr,
    bool* pbNeedsPreviewRender = nullptr)
{"""

content = content.replace(old_sig, new_sig)

# Update visibility click
old_vis = """        if (ImGui::Button((layer.Enabled ? "[o]##vis" + std::to_string(i) : "[-]##vis" + std::to_string(i)).c_str())) {
            layer.Enabled = !layer.Enabled;
            bNeedsMapUpdate = true;
            changed = true;
        }"""

new_vis = """        if (ImGui::Button((layer.Enabled ? "[o]##vis" + std::to_string(i) : "[-]##vis" + std::to_string(i)).c_str())) {
            layer.Enabled = !layer.Enabled;
            if (pbNeedsPreviewRender) *pbNeedsPreviewRender = true;
            else bNeedsMapUpdate = true;
            changed = true;
        }"""

content = content.replace(old_vis, new_vis)

with open("gui/UIHelpers.h", "w", encoding="utf-8") as f:
    f.write(content)

// TextInput_UI.cpp — the imgui draw path of the shared text field. Layer: UI.
// One of the translation units that include imgui.h; the length cap, the character rules and the
// live/settled split are pure and live in the header (see WidgetHelpers_UI.h "THE SPLIT"), so this
// file is only the staging buffer and one imgui call. Rendering is verified by eye against a live
// frame, never by test.
#include "TextInput_UI.h"
#include "RtToggleWidget_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// imgui edits a char buffer, the caller owns a std::string, so each frame stages one into the
// other. The buffer is a LOCAL of the draw call — never a function static — because a widget in
// this library holds no state between frames (WidgetHelpers_UI.h). Refilling it from `value` every
// frame is safe precisely because the interaction step writes `value` back from it on the same
// frame, so the staged text and the caller's string never disagree.
int StageTextIntoBuffer(const std::string& value, const TextInputRules& rules,
                        char* buffer, int bufferCapacity) {
    const int maximumLength = ResolvedTextInputLength(rules);
    const int capacity = maximumLength + 1 < bufferCapacity ? maximumLength + 1 : bufferCapacity;
    int writtenLength = 0;
    while (writtenLength < capacity - 1 && writtenLength < static_cast<int>(value.size())) {
        buffer[writtenLength] = value[static_cast<std::string::size_type>(writtenLength)];
        ++writtenLength;
    }
    buffer[writtenLength] = '\0';
    return capacity;
}

} // namespace

WidgetChange DrawTextInput(const char* label, std::string& value, const TextInputRules& rules,
                           const WidgetStyle& style, const char* hintText) {
    char stagingBuffer[kTextInputBufferCapacity];
    const int capacity = StageTextIntoBuffer(value, rules, stagingBuffer, kTextInputBufferCapacity);

    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ResolveWidgetRounding(style));
    TextInputSignal signal;
    signal.bTextEditedThisFrame = (hintText != nullptr)
        ? ImGui::InputTextWithHint("##text", hintText, stagingBuffer, static_cast<size_t>(capacity))
        : ImGui::InputText("##text", stagingBuffer, static_cast<size_t>(capacity));
    signal.bEditFinishedThisFrame = ImGui::IsItemDeactivatedAfterEdit();
    ImGui::PopStyleVar();

    const WidgetChange change = StepTextInputInteraction(value, std::string(stagingBuffer), rules, signal);
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen

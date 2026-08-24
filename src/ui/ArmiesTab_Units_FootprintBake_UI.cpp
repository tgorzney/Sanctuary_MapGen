// ArmiesTab_Units_FootprintBake_UI.cpp — ARCH §1.5 aspect split off ArmiesTab_Units_UI.cpp (which
// hit the file-size ceiling): the "Resolve Footprint" bake button DrawArmyUnitList calls.
// Layer: UI. work_orders/STEP96_FootprintBakeAndStalenessCheck_IO.md §2. Declared on
// ArmiesTab_Units_UI.h; needs no header of its own (same posture as TemplateIngest_Report_IO.cpp's
// own comment on this pattern).
#include "ArmiesTab_Units_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The bounded tpId-buffer -> std::string conversion every wire-mapping/lookup site already
// re-implements locally (see PropsTab_Rules_UI.cpp's identical copy).
std::string BoundedTemplateIdentifierText(const char (&templateIdentifier)[8]) {
    std::size_t length = 0;
    while (length < sizeof(templateIdentifier) && templateIdentifier[length] != '\0') ++length;
    return std::string(templateIdentifier, length);
}

} // namespace

// STEP96_FootprintBakeAndStalenessCheck_IO.md §2 — a discrete, per-rule bake button, never
// automatic: fires only on click, never inside dirty-hash recompute, never notifies PreviewDriver.
void DrawResolveUnitFootprintButton(Params::UnitRule& rule, ArmyUnitListState& state,
                                    const Io::TemplateIngestReport* templateIngestReport) {
    const bool bHasTemplateIdentifier = rule.transform.templateIdentifier[0] != '\0';
    ImGui::BeginDisabled(!bHasTemplateIdentifier);
    const bool bClicked = ImGui::Button("Resolve Footprint");
    ImGui::EndDisabled();
    if (bClicked && bHasTemplateIdentifier) {
        const std::string templateIdentifier =
            BoundedTemplateIdentifierText(rule.transform.templateIdentifier);
        const Io::TemplateFootprintRecord* const record = templateIngestReport == nullptr
            ? nullptr : templateIngestReport->FindByTemplateIdentifier(templateIdentifier);
        if (ApplyResolvedFootprintBake(rule, record)) {
            state.bakeFootprintMessage.clear();
        } else {
            state.bakeFootprintMessage = "No ingested data for tpId '" + templateIdentifier
                + "'. Ingest game templates in the System tab, or enter a value by hand.";
        }
    }
    if (!state.bakeFootprintMessage.empty()) ImGui::TextWrapped("%s", state.bakeFootprintMessage.c_str());
}

} // namespace Ui
} // namespace SanmapGen

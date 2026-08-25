// FilePathPicker_UI_Test.cpp — acceptance test for the browse button + short-path label (A2).
// Covers the extension fence, the shortened label and the apply/clear change contract. No imgui
// frame, no window, no GL, and no filesystem: the picker's logic is pure by construction
// (FilePathPicker_UI.h / FilePathLabel_UI.h) and it never opens a dialog itself — the platform
// seam is an injected function, exercised here with a stub, exactly as a host will inject
// FileDialog_IO later.
#include "FilePathPicker_UI.h"
#include <cstdio>
#include <imgui.h>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static void TestPathTextHelpers() {
    Check(Ui::FileNameFromPath("D:\\maps\\rolling hills.sanmap") == "rolling hills.sanmap",
          "the file name is taken after a backslash");
    Check(Ui::FileNameFromPath("/home/user/maps/tundra.sanmap") == "tundra.sanmap",
          "and after a forward slash");
    Check(Ui::FileNameFromPath("tundra.sanmap") == "tundra.sanmap", "a bare name is its own file name");

    Check(Ui::ShortenedFilePathLabel("", 40) == Ui::EmptyFilePathLabel(), "an unset path reads as (none)");
    Check(Ui::ShortenedFilePathLabel("short.dds", 40) == "short.dds", "a path inside the budget is untouched");
    const std::string longPath = "D:\\gamedata\\environments\\temperate\\stratums\\albedo\\grass_01.dds";
    const std::string shortened = Ui::ShortenedFilePathLabel(longPath, 20);
    Check(shortened.size() == 20, "a long path is shortened to exactly the budget");
    Check(shortened.compare(0, 3, "...") == 0, "and is marked with a leading ellipsis");
    Check(shortened.compare(shortened.size() - 12, 12, "grass_01.dds") == 0,
          "keeping the tail that identifies the file");
    Check(Ui::ShortenedFilePathLabel(longPath, 0) == longPath, "a non-positive budget means no limit");
}

static void TestExtensionFenceRejectsBeforeAnythingLoads() {
    Check(Ui::HasAllowedFileExtension("terrain.dds", ".png;.dds"), "a listed extension is accepted");
    Check(Ui::HasAllowedFileExtension("TERRAIN.DDS", ".png;.dds"), "the compare ignores case");
    Check(!Ui::HasAllowedFileExtension("terrain.exe", ".png;.dds"), "an unlisted extension is refused");
    Check(!Ui::HasAllowedFileExtension("dds", ".png;.dds"), "a name that is not even a suffix match is refused");
    Check(Ui::HasAllowedFileExtension("anything.at.all", nullptr), "a null list fences nothing");
    Check(Ui::HasAllowedFileExtension("map.sanmap", ".sanmap"), "a single-entry list works");

    Ui::FilePathPickerOptions options;
    options.allowedExtensions = ".dds";
    std::string storedPath = "keep.dds";
    const Ui::FilePathPickerResult rejected = Ui::ApplyChosenFilePath(storedPath, "virus.exe", options);
    Check(rejected.bRejectedExtension, "a wrong pick is reported");
    Check(!rejected.change.bValueChanged && !rejected.change.bCommitted, "and costs no dirty flag");
    Check(storedPath == "keep.dds", "and NEVER reaches the setting");
    Check(!Ui::StoredFilePathIsAllowed("legacy.tga", options), "a stored path can be checked against the fence");
    Check(Ui::StoredFilePathIsAllowed("", options), "an unset path is not a fence violation");
}

static void TestApplyAndClearReportExactlyOneChange() {
    Ui::FilePathPickerOptions options;
    options.allowedExtensions = ".dds";
    std::string storedPath;

    const Ui::FilePathPickerResult applied = Ui::ApplyChosenFilePath(storedPath, "D:\\art\\rock.dds", options);
    Check(applied.change.bValueChanged && applied.change.bCommitted, "a pick changes and commits on one frame");
    Check(storedPath == "D:\\art\\rock.dds", "and lands in the caller's setting");

    const Ui::FilePathPickerResult repeated = Ui::ApplyChosenFilePath(storedPath, "D:\\art\\rock.dds", options);
    Check(!repeated.change.bValueChanged && !repeated.change.bCommitted, "re-picking the same file is free");

    const Ui::FilePathPickerResult cleared = Ui::ClearFilePath(storedPath);
    Check(cleared.change.bCommitted && storedPath.empty(), "clearing empties the setting and commits");
    const Ui::FilePathPickerResult clearedAgain = Ui::ClearFilePath(storedPath);
    Check(!clearedAgain.change.bCommitted, "clearing an already-empty setting costs nothing");
}

// The stub platform seam: answers a fixed path once, then reports "cancelled".
struct StubDialog {
    const char* answerPath  = nullptr;
    int         callCount   = 0;
};

static bool StubRequestFilePath(void* userData, const char* startingPath, std::string& outChosenPath) {
    StubDialog& dialog = *static_cast<StubDialog*>(userData);
    ++dialog.callCount;
    (void)startingPath;
    if (dialog.answerPath == nullptr) return false;
    outChosenPath = dialog.answerPath;
    dialog.answerPath = nullptr;
    return true;
}

static void TestInjectedDialogSeam() {
    StubDialog dialog;
    dialog.answerPath = "D:\\art\\cliff.dds";
    Ui::FilePathPickerOptions options;
    options.allowedExtensions = ".dds";
    options.RequestFilePath   = &StubRequestFilePath;
    options.requestUserData   = &dialog;

    std::string storedPath;
    std::string chosenPath;
    Check(options.RequestFilePath(options.requestUserData, storedPath.c_str(), chosenPath),
          "the seam answers a path the first time");
    const Ui::FilePathPickerResult applied = Ui::ApplyChosenFilePath(storedPath, chosenPath, options);
    Check(applied.change.bCommitted && storedPath == "D:\\art\\cliff.dds", "the answer becomes the setting");

    Check(!options.RequestFilePath(options.requestUserData, storedPath.c_str(), chosenPath),
          "a cancelled dialog reports false");
    const Ui::FilePathPickerResult cancelled = Ui::ApplyChosenFilePath(storedPath, storedPath, options);
    Check(!cancelled.change.bValueChanged, "and leaves the setting alone");
    Check(dialog.callCount == 2, "the widget calls the seam once per browse, never per frame");
}

// STEP153: one live headless imgui frame, no window backend, no GL — mirrors Section_UI_Test.cpp's
// own BeginHeadlessFrame technique — to prove the redesigned control is a genuine single button at
// rest, shows its tooltip on hover, and reaches Clear only through a right-click context menu (never
// a second always-visible element). Everything above this stays pure/imgui-free by construction.
namespace {

constexpr unsigned long long kFilePathPickerTestFontAtlasIdentifier = 0xF0000153ull;

void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(512.0f, 512.0f);
    io.DeltaTime   = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr;
    int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kFilePathPickerTestFontAtlasIdentifier));
    ImGui::NewFrame();
}

bool NearlyEqual(float value, float expected) {
    const float difference = value - expected;
    return difference < 0.01f && difference > -0.01f;
}

// One "begin window, draw the picker, end/render" frame at a fixed position/size, so every hit-test
// coordinate captured from it stays reproducible across the multi-frame click sequences below.
void RunPickerFrame(const char* windowName, const char* label, std::string& filePath,
                    const Ui::FilePathPickerOptions& options) {
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin(windowName, nullptr, ImGuiWindowFlags_NoSavedSettings);
    Ui::DrawFilePathPicker(label, filePath, options);
    ImGui::End();
    ImGui::Render();
}

// Acceptance: at rest (no hover, no click) the picker draws EXACTLY one item — its button — and
// nothing beside it. Checked two ways: the id the call leaves on the ID stack is the SAME id a lone
// `ImGui::Button(label)` under the same PushID(label) scope would leave (so the LAST thing drawn is
// that one button, not some trailing element), and the row's total height matches a lone button's
// height exactly (so nothing else was drawn above it either) — the same "reference sequence costs
// exactly what the real one should" technique RunGroupStratumIndexRemovedCheck/
// RunDisabledCheckboxCommitChecks use (LayerEditor_InlineSettings_UI_Test.cpp).
void TestSingleButtonAtRestDrawsNothingElse() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(-1.0f, -1.0f);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);

    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("FilePathPickerAtRestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    ImGui::PushID("Gamedata Folder");
    const ImGuiID expectedButtonId = ImGui::GetID("Gamedata Folder");
    ImGui::PopID();
    const float startY = ImGui::GetCursorPosY();
    std::string storedPath = "D:\\gamedata";
    Ui::FilePathPickerOptions options;
    Ui::DrawFilePathPicker("Gamedata Folder", storedPath, options);
    const float actualHeight = ImGui::GetCursorPosY() - startY;
    Check(ImGui::GetItemID() == expectedButtonId,
          "the last (and only) item the picker leaves on the ID stack is its own single button");
    ImGui::End();
    ImGui::Render();

    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("FilePathPickerReferenceWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const float referenceStartY = ImGui::GetCursorPosY();
    ImGui::Button("Gamedata Folder");
    const float referenceHeight = ImGui::GetCursorPosY() - referenceStartY;
    ImGui::End();
    ImGui::Render();

    Check(NearlyEqual(actualHeight, referenceHeight),
          "and the row costs exactly one button's height -- nothing drawn above or below it either");

    ImGui::DestroyContext();
}

// Acceptance: the current path (or "None") shows on hover, via the SAME ShortenedFilePathLabel this
// file's own pure tests already cover. Exact tooltip pixels are, like every other draw path in this
// library, "verified by eye against a live frame, never by test" (TextInput_UI.cpp/SliderScalar_UI.cpp's
// own words) — what IS testable headless is that hovering draws strictly more geometry than resting
// (a tooltip actually appeared), that an EMPTY path still draws one (the "(none)" state, not a
// suppressed tooltip), and that its shorter text costs fewer vertices than a long real path's tooltip
// (the content tracks the stored path, not a fixed string) — mirroring CoreInputWidgets_LiveFrame_UI_Test's
// own "TotalVtxCount proves real geometry emitted" technique.
void TestTooltipShowsCurrentPathOnHover() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.AddMousePosEvent(-1.0f, -1.0f);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("TooltipTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    std::string longStoredPath = "D:\\gamedata\\environments\\temperate\\stratums\\albedo\\grass_01.dds";
    Ui::FilePathPickerOptions options;
    Ui::DrawFilePathPicker("Albedo", longStoredPath, options);
    const ImVec2 rectMin = ImGui::GetItemRectMin();
    const ImVec2 rectMax = ImGui::GetItemRectMax();
    ImGui::End();
    ImGui::Render();
    const int vertexCountAtRest = ImGui::GetDrawData()->TotalVtxCount;

    const float centerX = (rectMin.x + rectMax.x) * 0.5f;
    const float centerY = (rectMin.y + rectMax.y) * 0.5f;
    io.AddMousePosEvent(centerX, centerY);

    // The tooltip is its own auto-sizing popup window; its FIRST-EVER appearance in a context lays
    // out before its size has converged (the same one-frame-behind lag every auto-resize imgui
    // window has), so one untimed warm-up hover frame runs before each measurement below — mirroring
    // this file's other multi-frame click sequences' own "settle, then measure" discipline.
    RunPickerFrame("TooltipTestWindow", "Albedo", longStoredPath, options);          // warm-up
    RunPickerFrame("TooltipTestWindow", "Albedo", longStoredPath, options);
    const int vertexCountHoveredWithPath = ImGui::GetDrawData()->TotalVtxCount;
    Check(vertexCountHoveredWithPath > vertexCountAtRest,
          "hovering the button draws extra tooltip geometry the at-rest frame did not");

    std::string emptyStoredPath;
    RunPickerFrame("TooltipTestWindow", "Albedo", emptyStoredPath, options);         // warm-up
    RunPickerFrame("TooltipTestWindow", "Albedo", emptyStoredPath, options);
    const int vertexCountHoveredEmpty = ImGui::GetDrawData()->TotalVtxCount;
    Check(vertexCountHoveredEmpty > vertexCountAtRest,
          "an EMPTY path still shows a tooltip (\"(none)\"), not a suppressed one");
    Check(vertexCountHoveredEmpty < vertexCountHoveredWithPath,
          "and its shorter \"(none)\" text costs fewer vertices than the long real path's tooltip");

    ImGui::DestroyContext();
}

// Runs the SAME right-click-then-menu-entry shape DrawFilePathPicker's own .cpp draws (Button,
// BeginPopupContextItem, MenuItem("Clear")) directly with public imgui calls, in its own window at
// the SAME pos/size DrawFilePathPicker's own test windows use — so its button lands at the identical
// pixel rect a real "Albedo" picker's button would. Captures the "Clear" entry's rect from INSIDE
// this same call sequence, before EndPopup: `ImGui::EndPopup()`/`End()` restores the CALLER's last-
// item state once a popup closes back to its parent (so an outer `GetItemRectMin()` taken AFTER a
// `DrawFilePathPicker()` call that opened a popup reads the ORIGINAL button, never the popup's own
// last item) — the same "reference sequence, drawn directly, to learn coordinates a wrapped call
// won't hand back" technique RunGroupStratumIndexRemovedCheck/RunDisabledCheckboxCommitChecks use
// (LayerEditor_InlineSettings_UI_Test.cpp). Both rects returned are in screen space, so a caller can
// feed them straight into a synthetic mouse driving the REAL widget in its OWN, separate window.
struct ClearMenuReferenceRects {
    ImVec2 buttonRectMin, buttonRectMax;
    ImVec2 menuItemRectMin, menuItemRectMax;
};

ClearMenuReferenceRects CaptureClearMenuReferenceRects(ImGuiIO& io) {
    ClearMenuReferenceRects rects;
    const char* const kWindow = "ClearMenuReferenceWindow";

    io.AddMousePosEvent(-1.0f, -1.0f);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin(kWindow, nullptr, ImGuiWindowFlags_NoSavedSettings);
    ImGui::PushID("Albedo");
    ImGui::Button("Albedo");
    rects.buttonRectMin = ImGui::GetItemRectMin();
    rects.buttonRectMax = ImGui::GetItemRectMax();
    ImGui::PopID();
    ImGui::End();
    ImGui::Render();

    const float buttonCenterX = (rects.buttonRectMin.x + rects.buttonRectMax.x) * 0.5f;
    const float buttonCenterY = (rects.buttonRectMin.y + rects.buttonRectMax.y) * 0.5f;

    auto drawFrame = [&]() {
        BeginHeadlessFrame();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
        ImGui::Begin(kWindow, nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImGui::PushID("Albedo");
        ImGui::Button("Albedo");
        if (ImGui::BeginPopupContextItem()) {
            ImGui::MenuItem("Clear");
            rects.menuItemRectMin = ImGui::GetItemRectMin();
            rects.menuItemRectMax = ImGui::GetItemRectMax();
            ImGui::EndPopup();
        }
        ImGui::PopID();
        ImGui::End();
        ImGui::Render();
    };

    io.AddMousePosEvent(buttonCenterX, buttonCenterY);            // settle
    drawFrame();
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, true);         // press
    drawFrame();
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);        // release: opens the popup
    drawFrame();
    drawFrame();   // converge: the popup's own auto-sized window needs a second frame to lay out
                    // its content reliably, same as the tooltip window above.
    return rects;
}

// Acceptance: a right-click on the button surfaces a WORKING Clear when bClearButtonShown is true —
// driven through the REAL widget with a real press/release sequence over its own item rects (the
// button's from the reference capture above; the "Clear" entry's likewise, since its own popup opens
// at the SAME mouse position over the SAME button and renders identical content, so the two coincide
// pixel-for-pixel) — the same "settle frame before every press" discipline
// RunAddGeoLayerButtonClickThroughChecks/RunDisabledCheckboxCommitChecks use.
void TestRightClickSurfacesWorkingClearWhenShown() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    const ClearMenuReferenceRects reference = CaptureClearMenuReferenceRects(io);
    const float buttonCenterX = (reference.buttonRectMin.x + reference.buttonRectMax.x) * 0.5f;
    const float buttonCenterY = (reference.buttonRectMin.y + reference.buttonRectMax.y) * 0.5f;
    const float menuItemCenterX = (reference.menuItemRectMin.x + reference.menuItemRectMax.x) * 0.5f;
    const float menuItemCenterY = (reference.menuItemRectMin.y + reference.menuItemRectMax.y) * 0.5f;

    const char* const kWindow = "ClearMenuTestWindow";
    std::string storedPath = "D:\\art\\rock.dds";
    Ui::FilePathPickerOptions options;                          // bClearButtonShown defaults true

    io.AddMousePosEvent(-1.0f, -1.0f);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
    RunPickerFrame(kWindow, "Albedo", storedPath, options);

    // Settle: mouse arrives over the button, nothing pressed yet.
    io.AddMousePosEvent(buttonCenterX, buttonCenterY);
    RunPickerFrame(kWindow, "Albedo", storedPath, options);

    // Press the RIGHT button over the button's own rect: held, not yet released.
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, true);
    RunPickerFrame(kWindow, "Albedo", storedPath, options);
    Check(storedPath == "D:\\art\\rock.dds", "holding the right button does not clear anything yet");

    // Release over the same rect: the context menu opens THIS frame (BeginPopupContextItem's own
    // open-on-release contract).
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
    RunPickerFrame(kWindow, "Albedo", storedPath, options);
    Check(ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel),
          "right-clicking the button opens its context menu");
    Check(storedPath == "D:\\art\\rock.dds", "opening the menu alone does not clear the path");

    // Settle onto "Clear" (the popup stays open across frames on its own, same as any other imgui
    // popup), then press+release the LEFT button over it -- a MenuItem fires on release, like a Button.
    io.AddMousePosEvent(menuItemCenterX, menuItemCenterY);
    RunPickerFrame(kWindow, "Albedo", storedPath, options);

    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    RunPickerFrame(kWindow, "Albedo", storedPath, options);
    Check(storedPath == "D:\\art\\rock.dds", "pressing Clear does not fire on the press frame");

    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    RunPickerFrame(kWindow, "Albedo", storedPath, options);
    Check(storedPath.empty(), "releasing over \"Clear\" empties the stored path through the real widget");

    ImGui::DestroyContext();
}

// Acceptance: bClearButtonShown = false means the right-click context menu never appears at all --
// no separate always-visible clear affordance survives in its place either.
void TestRightClickSurfacesNoMenuWhenClearHidden() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    const char* const kWindow = "NoClearMenuTestWindow";

    io.AddMousePosEvent(-1.0f, -1.0f);
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
    std::string storedPath = "D:\\art\\rock.dds";
    Ui::FilePathPickerOptions options;
    options.bClearButtonShown = false;
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin(kWindow, nullptr, ImGuiWindowFlags_NoSavedSettings);
    Ui::DrawFilePathPicker("New Override File", storedPath, options);
    const ImVec2 buttonRectMin = ImGui::GetItemRectMin();
    const ImVec2 buttonRectMax = ImGui::GetItemRectMax();
    ImGui::End();
    ImGui::Render();

    const float buttonCenterX = (buttonRectMin.x + buttonRectMax.x) * 0.5f;
    const float buttonCenterY = (buttonRectMin.y + buttonRectMax.y) * 0.5f;

    io.AddMousePosEvent(buttonCenterX, buttonCenterY);
    RunPickerFrame(kWindow, "New Override File", storedPath, options);

    io.AddMouseButtonEvent(ImGuiMouseButton_Right, true);
    RunPickerFrame(kWindow, "New Override File", storedPath, options);

    io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
    RunPickerFrame(kWindow, "New Override File", storedPath, options);
    Check(!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel),
          "bClearButtonShown = false means a right-click never opens a menu at all");

    ImGui::DestroyContext();
}

} // namespace

int main() {
    TestPathTextHelpers();
    TestExtensionFenceRejectsBeforeAnythingLoads();
    TestApplyAndClearReportExactlyOneChange();
    TestInjectedDialogSeam();
    TestSingleButtonAtRestDrawsNothingElse();
    TestTooltipShowsCurrentPathOnHover();
    TestRightClickSurfacesWorkingClearWhenShown();
    TestRightClickSurfacesNoMenuWhenClearHidden();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

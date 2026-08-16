// FilePathPicker_UI_Test.cpp — acceptance test for the browse button + short-path label (A2).
// Covers the extension fence, the shortened label and the apply/clear change contract. No imgui
// frame, no window, no GL, and no filesystem: the picker's logic is pure by construction
// (FilePathPicker_UI.h / FilePathLabel_UI.h) and it never opens a dialog itself — the platform
// seam is an injected function, exercised here with a stub, exactly as a host will inject
// FileDialog_IO later.
#include "FilePathPicker_UI.h"
#include <cstdio>

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

int main() {
    TestPathTextHelpers();
    TestExtensionFenceRejectsBeforeAnythingLoads();
    TestApplyAndClearReportExactlyOneChange();
    TestInjectedDialogSeam();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

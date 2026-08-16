// FileDialog_IO_Test.cpp — acceptance test for the native picker seam (section D).
// It deliberately never SHOWS a dialog: a headless test that popped a window would hang CTest.
// Everything asserted here is the part that runs BEFORE the shell is touched — the request fence
// and the extension normalization — plus the contract that a refused request leaves the caller's
// string exactly as it was (Constitution §6: a cancel can never blank a setting).
#include "FileDialog_IO.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static void TestDefaultExtensionLosesItsDots() {
    Check(Io::NormalizedDefaultExtension(nullptr).empty(), "a null default extension is empty");
    Check(Io::NormalizedDefaultExtension("") .empty(), "an empty default extension stays empty");
    Check(Io::NormalizedDefaultExtension("sanmap") == "sanmap", "a bare extension is unchanged");
    Check(Io::NormalizedDefaultExtension(".sanmap") == "sanmap", "the settings-style dot is stripped");
    Check(Io::NormalizedDefaultExtension("..raw") == "raw", "and so is a doubled one");
}

static void TestRequestFenceCatchesAWildFilterArray() {
    Io::FileDialogRequest request;
    Check(request.IsValid(), "a default request (no filters at all) is usable");

    request.filterCount = 2;                 // count without an array — the one wild-pointer path
    Check(!request.IsValid(), "a filter count with no array is refused");

    const Io::FileDialogFilter filters[] = { { "Sanctuary map", "*.sanmap" } };
    request.filters = filters;
    request.filterCount = 1;
    Check(request.IsValid(), "a matching array and count is usable");

    request.filterCount = -1;
    Check(!request.IsValid(), "a negative count is refused");
}

static void TestARefusedRequestNeverTouchesTheCallersPath() {
    Io::FileDialogRequest request;
    request.filterCount = 3;                 // invalid: refused before any shell call
    Check(!request.IsValid(), "the fixture request really is invalid");

    std::string chosenPath = "D:/existing/setting.sanmap";
    const std::string originalPath = chosenPath;
    Check(!Io::FileDialog::OpenFilePath(request, chosenPath), "OpenFilePath refuses it");
    Check(!Io::FileDialog::SaveFilePath(request, chosenPath), "SaveFilePath refuses it");
    Check(!Io::FileDialog::SelectDirectoryPath(request, chosenPath), "SelectDirectoryPath refuses it");
    Check(chosenPath == originalPath, "and none of the three blanked the caller's setting");
}

static void TestAvailabilityIsAnswerable() {
    // Not asserted either way — a headless build legitimately has no picker. What matters is that
    // a host can ASK, so it can fall back to a typed path instead of appearing to do nothing.
    const bool bAvailable = Io::IsNativeFileDialogAvailable();
    std::printf("native file dialog available: %s\n", bAvailable ? "yes" : "no");
#ifdef _WIN32
    Check(bAvailable, "the Win32 shell seam is compiled in on Windows");
#else
    Check(!bAvailable, "a platform with no seam reports so rather than lying");
#endif
}

int main() {
    TestDefaultExtensionLosesItsDots();
    TestRequestFenceCatchesAWildFilterArray();
    TestARefusedRequestNeverTouchesTheCallersPath();
    TestAvailabilityIsAnswerable();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

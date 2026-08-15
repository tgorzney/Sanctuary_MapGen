// GpuResource_ShaderPath_SYS_Test.cpp — acceptance checks for the shader search path (M5-0b):
// two kernels in two different directories both resolve, the scan is ORDERED (first match
// wins), and a kernel on no directory fails cleanly instead of throwing. All directories are
// derived from the directory the harness is handed — never a hardcoded absolute path.
#include "GpuResource_SYS.h"
#include "GpuResource_TestSupport_SYS.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace SanmapGen::Sys;

namespace GpuResourceTest {

namespace {

std::string MakeTrivialKernel(const char* writtenValue) {
    return std::string("#version 430 core\n")
         + "layout(local_size_x = 1) in;\n"
         + "layout(std430, binding = 0) buffer Data { float values[]; };\n"
         + "void main() { values[0] = " + writtenValue + "; }\n";
}

} // namespace

void WriteTestFile(const std::string& filePath, const std::string& contents) {
    std::ofstream out(filePath);
    out << contents;
}

void RunShaderSearchPathChecks(const std::string& baseDirectory) {
    // Two directories stand in for the co-located src/proc and src/ui kernel folders.
    const std::string firstDirectory  = baseDirectory + "/GpuResourceSearchPathFirst";
    const std::string secondDirectory = baseDirectory + "/GpuResourceSearchPathSecond";
    std::filesystem::create_directories(firstDirectory);
    std::filesystem::create_directories(secondDirectory);
    WriteTestFile(firstDirectory  + "/SearchPathFirst_Test.glsl",    MakeTrivialKernel("1.0"));
    WriteTestFile(secondDirectory + "/SearchPathSecond_Test.glsl",   MakeTrivialKernel("2.0"));
    // Same file name in both directories: the first is valid, the second will not compile,
    // so which one the loader picked is observable.
    WriteTestFile(firstDirectory  + "/SearchPathShadowed_Test.glsl", MakeTrivialKernel("3.0"));
    WriteTestFile(secondDirectory + "/SearchPathShadowed_Test.glsl",
                  "#version 430 core\nthis line is not GLSL\n");

    GpuResourceManager manager(std::vector<std::string>{ firstDirectory, secondDirectory });
    Check(manager.Initialize(), "search-path manager initializes against the GL loader");
    Check(manager.GetOrCompileProgram("SearchPathFirst_Test.glsl").IsValid(),
          "kernel resolves from the first directory on the path");
    Check(manager.GetOrCompileProgram("SearchPathSecond_Test.glsl").IsValid(),
          "kernel resolves from the second directory on the path");
    Check(manager.GetOrCompileProgram("SearchPathShadowed_Test.glsl").IsValid(),
          "shadowed name resolves to the first match on the path");
    std::printf("(the next two GpuResourceManager errors are expected by this test)\n");
    Check(!manager.GetOrCompileProgram("NotOnTheSearchPath_Test.glsl").IsValid(),
          "a kernel on no directory of the path fails cleanly");

    // Reversed order must reach the OTHER twin — proving an ordered scan, not a merge.
    GpuResourceManager reversedManager(std::vector<std::string>{ secondDirectory, firstDirectory });
    Check(reversedManager.Initialize(), "reversed-path manager initializes");
    Check(!reversedManager.GetOrCompileProgram("SearchPathShadowed_Test.glsl").IsValid(),
          "reversing the path order resolves the other twin (scan is ordered)");
}

} // namespace GpuResourceTest

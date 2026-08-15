// NoiseBlend_GpuParity_PROC_Test.cpp — acceptance item 1 of work-order M3-1: the Cpu accuracy
// path and the Gpu speed path, handed the SAME layer configuration, must agree within the
// stage's Visual-class tolerance (documented in NoiseBlend_TestSupport_PROC.h). Owns the GL
// harness — a hidden window, because this is a test, not app code — and drives the other Gpu
// checks; with no GL context it reports SKIP and fails nothing.
#include "NoiseBlend_PROC.h"
#include "NoiseBlend_TestStacks_PROC.h"
#include "NoiseBlend_TestSupport_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include "../sys/GpuGlFunctions_SYS.h"
#include <cstdio>

using namespace SanmapGen;

void NoiseBlendCheck(bool bPassed, const char* label);             // NoiseBlend_PROC_Test.cpp
void CheckGpuBlendModes(Sys::GpuResourceManager& manager);         // NoiseBlend_GpuBlend_PROC_Test.cpp
void CheckDispatchRouting(Sys::GpuResourceManager& manager);       // NoiseBlend_GpuBlend_PROC_Test.cpp
void CheckNoiseTypeParity(Sys::GpuResourceManager& manager);       // NoiseBlend_GpuBlend_PROC_Test.cpp

namespace {

bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "NoiseBlendParityTestWindow";
    RegisterClassA(&windowClass);
    outWindow = CreateWindowExA(0, windowClass.lpszClassName, "hidden", 0, 0, 0, 8, 8,
                                nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!outWindow) return false;
    outDeviceContext = GetDC(outWindow);
    PIXELFORMATDESCRIPTOR descriptor = {};
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    int pixelFormat = ChoosePixelFormat(outDeviceContext, &descriptor);
    if (!pixelFormat || !SetPixelFormat(outDeviceContext, pixelFormat, &descriptor)) return false;
    outGlContext = wglCreateContext(outDeviceContext);
    return outGlContext && wglMakeCurrent(outDeviceContext, outGlContext);
}

void CheckStackParity(Sys::GpuResourceManager& manager) {
    Params::Geometry geometry;
    geometry.mapSize = 255;
    geometry.seed = 1337u;
    Params::LayerStack stack = Proc::MakeRepresentativeStack();
    Data::MapFields cpuFields;
    Data::MapFields gpuFields;
    Proc::NoiseBlendStage cpuStage(geometry, stack, cpuFields);
    Proc::NoiseBlendStage gpuStage(geometry, stack, gpuFields);
    gpuStage.SetGpuResourceManager(&manager);
    cpuStage.RunOnCpu();
    gpuStage.RunOnGpu();
    NoiseBlendCheck(gpuStage.LastBackend() == Sys::ComputeBackend::Gpu, "the Gpu path actually ran");
    NoiseBlendCheck(cpuStage.LayerConfigurations().size() == gpuStage.LayerConfigurations().size()
                    && !gpuStage.LayerConfigurations().empty(), "both backends got the same layer count");
    Proc::CheckFieldHasSignal(cpuFields.heightfield, "the Cpu heightfield varies", NoiseBlendCheck);
    Proc::CheckFieldHasSignal(gpuFields.heightfield, "the Gpu heightfield varies", NoiseBlendCheck);
    Proc::CheckFieldHasSignal(cpuFields.materialProportions[1], "materialProportion[1] varies", NoiseBlendCheck);
    Proc::CompareFields(cpuFields.heightfield, gpuFields.heightfield, "heightfield", NoiseBlendCheck);
    Proc::CompareFields(cpuFields.materialProportions[0], gpuFields.materialProportions[0], "materialProportion[0]", NoiseBlendCheck);
    Proc::CompareFields(cpuFields.materialProportions[1], gpuFields.materialProportions[1], "materialProportion[1]", NoiseBlendCheck);
    // The Gpu keeps the same two-level cache: an unchanged re-run touches nothing at all.
    gpuStage.RunOnGpu();
    NoiseBlendCheck(gpuStage.WasLastRunSkipped(), "an unchanged Gpu re-run is skipped");
}

} // namespace

void RunNoiseBlendGpuParityChecks(const char* shaderDirectory) {
    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("SKIP: no GL context available; Cpu/Gpu parity not verified\n");
        return;
    }
    Sys::GpuResourceManager manager(shaderDirectory);
    NoiseBlendCheck(manager.Initialize(), "GpuResourceManager initializes");
    CheckStackParity(manager);
    CheckNoiseTypeParity(manager);
    CheckGpuBlendModes(manager);
    CheckDispatchRouting(manager);
    std::printf("  gpu programCompiles=%d bufferReallocations=%d\n",
                manager.CompileCount(), manager.ReallocationCount());
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);
}

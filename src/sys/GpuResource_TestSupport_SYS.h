// GpuResource_TestSupport_SYS.h — shared harness for the GpuResource_SYS acceptance binary.
// The checks span several translation units (ARCH §1.5 ceilings) and every one of them needs
// the SAME live GL context, so the hidden-window WGL bring-up and the pass/fail counter live
// here and the context is created once in main(). Test scaffolding only — no shipping file
// includes this header.
#pragma once
#include "GpuGlFunctions_SYS.h"      // Windows.h + GL/gl.h, already NOMINMAX-guarded
#include <cstdio>
#include <string>

namespace GpuResourceTest {

inline int& FailureCount() { static int failureCount = 0; return failureCount; }

inline void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL: %s\n", label);
    ++FailureCount();
}

// Minimal hidden-window GL context so the manager has something to talk to.
inline bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "GpuResourceTestWindow";
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

// Writes a generated kernel next to the test's own scratch files (never an absolute path).
void WriteTestFile(const std::string& filePath, const std::string& contents);

// Defined in the sibling test translation units; both run against the current GL context.
void RunTextureChecks(const std::string& shaderDirectory);          // GpuResource_Texture_SYS_Test.cpp
void RunShaderSearchPathChecks(const std::string& baseDirectory);   // GpuResource_ShaderPath_SYS_Test.cpp

} // namespace GpuResourceTest

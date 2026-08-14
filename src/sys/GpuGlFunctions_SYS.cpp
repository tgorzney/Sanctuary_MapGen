// GpuGlFunctions_SYS.cpp — definitions + the one-time loader for the GL compute entry
// points (SYS). Mirrors the getProc fallback the legacy renderers used (wglGetProcAddress
// then opengl32.dll), consolidated so no other translation unit re-implements it.
#include "GpuGlFunctions_SYS.h"

namespace SanmapGen {
namespace Sys {

#define GPU_GL_DEFINE(type, name, symbol) type name = nullptr;
GPU_GL_FUNCTION_LIST(GPU_GL_DEFINE)
#undef GPU_GL_DEFINE

namespace {

bool g_bFunctionsLoaded = false;

void* ResolveEntryPoint(const char* name) {
    void* pointer = reinterpret_cast<void*>(wglGetProcAddress(name));
    // wglGetProcAddress returns these sentinel values for functions in the 1.1 core that
    // it will not hand back; fall through to the module export table in that case.
    if (pointer == nullptr || pointer == reinterpret_cast<void*>(0x1) ||
        pointer == reinterpret_cast<void*>(0x2) || pointer == reinterpret_cast<void*>(0x3) ||
        pointer == reinterpret_cast<void*>(-1)) {
        HMODULE openGlModule = GetModuleHandleA("opengl32.dll");
        pointer = reinterpret_cast<void*>(GetProcAddress(openGlModule, name));
    }
    return pointer;
}

} // namespace

bool LoadGpuGlFunctions() {
    if (g_bFunctionsLoaded) return true;
    if (LoadLibraryA("opengl32.dll") == nullptr) return false;

    bool bAllResolved = true;
#define GPU_GL_LOAD(type, name, symbol) \
    name = reinterpret_cast<type>(ResolveEntryPoint(symbol)); \
    if (name == nullptr) bAllResolved = false;
    GPU_GL_FUNCTION_LIST(GPU_GL_LOAD)
#undef GPU_GL_LOAD

    g_bFunctionsLoaded = bAllResolved;
    return bAllResolved;
}

} // namespace Sys
} // namespace SanmapGen

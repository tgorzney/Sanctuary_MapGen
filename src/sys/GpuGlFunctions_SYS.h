// GpuGlFunctions_SYS.h — the ONE place OpenGL compute entry points are loaded.
// Layer: SYS. Concentrates the GL type/enum/function-pointer boilerplate that used to
// be triplicated across ErosionCompute / TerrainCompute / PreviewRenderer (DISPATCH_
// INTERFACE_SPEC §3). Only GpuResource_SYS consumes these; no GL handle leaks upward.
// Requires a current GL context before LoadGpuGlFunctions() is called.
#pragma once
#define NOMINMAX
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <Windows.h>
#include <GL/gl.h>
#include <cstddef>

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

namespace SanmapGen {
namespace Sys {

// Types the bundled GL 1.1 header lacks.
typedef char GpuGlChar;
typedef ptrdiff_t GpuGlSizeiPointer;
typedef ptrdiff_t GpuGlIntPointer;
typedef unsigned long long GpuGlUnsigned64;
typedef struct GpuGlSyncOpaque* GpuGlSync;

// Enum constants used by the resource manager (values are stable GL tokens).
enum : GLenum {
    kGlComputeShader           = 0x91B9,
    kGlShaderStorageBuffer     = 0x90D2,
    kGlCompileStatus           = 0x8B81,
    kGlLinkStatus              = 0x8B82,
    kGlDynamicCopy             = 0x88EA,
    kGlShaderStorageBarrierBit = 0x2000,
    kGlSyncGpuCommandsComplete = 0x9117,
    kGlAlreadySignaled         = 0x911A,
    kGlConditionSatisfied      = 0x911C,
    kGlSyncFlushCommandsBit    = 0x00000001,
    // Texture / image-unit tokens (the managed textures in GpuResource_Texture_SYS.cpp).
    kGlRgba8                       = 0x8058,
    kGlClampToEdge                 = 0x812F,
    kGlTextureUnitZero             = 0x84C0,
    kGlReadOnly                    = 0x88B8,
    kGlWriteOnly                   = 0x88B9,
    kGlReadWrite                   = 0x88BA,
    kGlShaderImageAccessBarrierBit = 0x00000020,
    kGlTextureUpdateBarrierBit     = 0x00000100,
};

// Function-pointer typedefs.
typedef GLuint (APIENTRYP FnGlCreateShader)(GLenum);
typedef void   (APIENTRYP FnGlShaderSource)(GLuint, GLsizei, const GpuGlChar* const*, const GLint*);
typedef void   (APIENTRYP FnGlCompileShader)(GLuint);
typedef void   (APIENTRYP FnGlGetShaderiv)(GLuint, GLenum, GLint*);
typedef void   (APIENTRYP FnGlGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GpuGlChar*);
typedef GLuint (APIENTRYP FnGlCreateProgram)(void);
typedef void   (APIENTRYP FnGlAttachShader)(GLuint, GLuint);
typedef void   (APIENTRYP FnGlLinkProgram)(GLuint);
typedef void   (APIENTRYP FnGlGetProgramiv)(GLuint, GLenum, GLint*);
typedef void   (APIENTRYP FnGlGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GpuGlChar*);
typedef void   (APIENTRYP FnGlUseProgram)(GLuint);
typedef void   (APIENTRYP FnGlDeleteShader)(GLuint);
typedef void   (APIENTRYP FnGlDeleteProgram)(GLuint);
typedef void   (APIENTRYP FnGlGenBuffers)(GLsizei, GLuint*);
typedef void   (APIENTRYP FnGlBindBuffer)(GLenum, GLuint);
typedef void   (APIENTRYP FnGlBufferData)(GLenum, GpuGlSizeiPointer, const void*, GLenum);
typedef void   (APIENTRYP FnGlBufferSubData)(GLenum, GpuGlIntPointer, GpuGlSizeiPointer, const void*);
typedef void   (APIENTRYP FnGlGetBufferSubData)(GLenum, GpuGlIntPointer, GpuGlSizeiPointer, void*);
typedef void   (APIENTRYP FnGlBindBufferBase)(GLenum, GLuint, GLuint);
typedef void   (APIENTRYP FnGlDeleteBuffers)(GLsizei, const GLuint*);
typedef void   (APIENTRYP FnGlDispatchCompute)(GLuint, GLuint, GLuint);
typedef void   (APIENTRYP FnGlMemoryBarrier)(GLbitfield);
typedef GLint  (APIENTRYP FnGlGetUniformLocation)(GLuint, const GpuGlChar*);
typedef void   (APIENTRYP FnGlUniform1i)(GLint, GLint);
// Texture entry points past GL 1.1 (glGenTextures/glTexImage2D/glGetTexImage are core 1.1
// and are called directly through opengl32; only these two must be resolved).
typedef void   (APIENTRYP FnGlActiveTexture)(GLenum);
typedef void   (APIENTRYP FnGlBindImageTexture)(GLuint, GLuint, GLint, GLboolean, GLint, GLenum, GLenum);
typedef GpuGlSync (APIENTRYP FnGlFenceSync)(GLenum, GLbitfield);
typedef GLenum (APIENTRYP FnGlClientWaitSync)(GpuGlSync, GLbitfield, GpuGlUnsigned64);
typedef void   (APIENTRYP FnGlDeleteSync)(GpuGlSync);

// The single macro list drives extern declarations, definitions, and loading, so the set
// stays consistent in exactly one place. ENTRY(pointerType, pointerVariable, "glSymbol").
#define GPU_GL_FUNCTION_LIST(ENTRY) \
    ENTRY(FnGlCreateShader,      glCreateShaderPointer,      "glCreateShader") \
    ENTRY(FnGlShaderSource,      glShaderSourcePointer,      "glShaderSource") \
    ENTRY(FnGlCompileShader,     glCompileShaderPointer,     "glCompileShader") \
    ENTRY(FnGlGetShaderiv,       glGetShaderivPointer,       "glGetShaderiv") \
    ENTRY(FnGlGetShaderInfoLog,  glGetShaderInfoLogPointer,  "glGetShaderInfoLog") \
    ENTRY(FnGlCreateProgram,     glCreateProgramPointer,     "glCreateProgram") \
    ENTRY(FnGlAttachShader,      glAttachShaderPointer,      "glAttachShader") \
    ENTRY(FnGlLinkProgram,       glLinkProgramPointer,       "glLinkProgram") \
    ENTRY(FnGlGetProgramiv,      glGetProgramivPointer,      "glGetProgramiv") \
    ENTRY(FnGlGetProgramInfoLog, glGetProgramInfoLogPointer, "glGetProgramInfoLog") \
    ENTRY(FnGlUseProgram,        glUseProgramPointer,        "glUseProgram") \
    ENTRY(FnGlDeleteShader,      glDeleteShaderPointer,      "glDeleteShader") \
    ENTRY(FnGlDeleteProgram,     glDeleteProgramPointer,     "glDeleteProgram") \
    ENTRY(FnGlGenBuffers,        glGenBuffersPointer,        "glGenBuffers") \
    ENTRY(FnGlBindBuffer,        glBindBufferPointer,        "glBindBuffer") \
    ENTRY(FnGlBufferData,        glBufferDataPointer,        "glBufferData") \
    ENTRY(FnGlBufferSubData,     glBufferSubDataPointer,     "glBufferSubData") \
    ENTRY(FnGlGetBufferSubData,  glGetBufferSubDataPointer,  "glGetBufferSubData") \
    ENTRY(FnGlBindBufferBase,    glBindBufferBasePointer,    "glBindBufferBase") \
    ENTRY(FnGlDeleteBuffers,     glDeleteBuffersPointer,     "glDeleteBuffers") \
    ENTRY(FnGlDispatchCompute,   glDispatchComputePointer,   "glDispatchCompute") \
    ENTRY(FnGlMemoryBarrier,     glMemoryBarrierPointer,     "glMemoryBarrier") \
    ENTRY(FnGlGetUniformLocation,glGetUniformLocationPointer,"glGetUniformLocation") \
    ENTRY(FnGlUniform1i,         glUniform1iPointer,         "glUniform1i") \
    ENTRY(FnGlActiveTexture,     glActiveTexturePointer,     "glActiveTexture") \
    ENTRY(FnGlBindImageTexture,  glBindImageTexturePointer,  "glBindImageTexture") \
    ENTRY(FnGlFenceSync,         glFenceSyncPointer,         "glFenceSync") \
    ENTRY(FnGlClientWaitSync,    glClientWaitSyncPointer,    "glClientWaitSync") \
    ENTRY(FnGlDeleteSync,        glDeleteSyncPointer,        "glDeleteSync")

#define GPU_GL_DECLARE(type, name, symbol) extern type name;
GPU_GL_FUNCTION_LIST(GPU_GL_DECLARE)
#undef GPU_GL_DECLARE

// Loads every pointer above from a current GL context. Idempotent; returns true only if
// every required entry point resolved. Safe to call repeatedly.
bool LoadGpuGlFunctions();

} // namespace Sys
} // namespace SanmapGen

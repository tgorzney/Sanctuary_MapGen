#include "TerrainCompute.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#define NOMINMAX
#include <Windows.h>
#include <GL/gl.h>

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

// Minimal GL extensions
typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;
#define GL_COMPUTE_SHADER                 0x91B9
#define GL_SHADER_STORAGE_BUFFER          0x90D2
#define GL_SHADER_STORAGE_BARRIER_BIT     0x2000
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_READ_ONLY                      0x88B8

typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLDISPATCHCOMPUTEPROC) (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void (APIENTRYP PFNGLMEMORYBARRIERPROC) (GLbitfield barriers);
typedef void (APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRYP PFNGLBINDBUFFERBASEPROC) (GLenum target, GLuint index, GLuint buffer);
typedef void* (APIENTRYP PFNGLMAPBUFFERPROC) (GLenum target, GLenum access);
typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFERPROC) (GLenum target);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);

static PFNGLCREATESHADERPROC glCreateShaderT = nullptr;
static PFNGLSHADERSOURCEPROC glShaderSourceT = nullptr;
static PFNGLCOMPILESHADERPROC glCompileShaderT = nullptr;
static PFNGLGETSHADERIVPROC glGetShaderivT = nullptr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLogT = nullptr;
static PFNGLCREATEPROGRAMPROC glCreateProgramT = nullptr;
static PFNGLATTACHSHADERPROC glAttachShaderT = nullptr;
static PFNGLLINKPROGRAMPROC glLinkProgramT = nullptr;
static PFNGLUSEPROGRAMPROC glUseProgramT = nullptr;
static PFNGLDISPATCHCOMPUTEPROC glDispatchComputeT = nullptr;
static PFNGLMEMORYBARRIERPROC glMemoryBarrierT = nullptr;
static PFNGLGENBUFFERSPROC glGenBuffersT = nullptr;
static PFNGLBINDBUFFERPROC glBindBufferT = nullptr;
static PFNGLBUFFERDATAPROC glBufferDataT = nullptr;
static PFNGLBINDBUFFERBASEPROC glBindBufferBaseT = nullptr;
static PFNGLMAPBUFFERPROC glMapBufferT = nullptr;
static PFNGLUNMAPBUFFERPROC glUnmapBufferT = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocationT = nullptr;
static PFNGLUNIFORM1IPROC glUniform1iT = nullptr;
static PFNGLDELETESHADERPROC glDeleteShaderT = nullptr;
static PFNGLDELETEPROGRAMPROC glDeleteProgramT = nullptr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffersT = nullptr;

static bool s_GLTInitialized = false;

static void LoadGLExtensionsT() {
    if (s_GLTInitialized) return;
    HMODULE libGL = LoadLibraryA("opengl32.dll");
    if(!libGL) return;

    auto getProc = [](const char* name) -> void* {
        void* p = (void*)wglGetProcAddress(name);
        if(p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1)) {
            HMODULE module = GetModuleHandleA("opengl32.dll");
            p = (void*)GetProcAddress(module, name);
        }
        return p;
    };

    glCreateShaderT = (PFNGLCREATESHADERPROC)getProc("glCreateShader");
    glShaderSourceT = (PFNGLSHADERSOURCEPROC)getProc("glShaderSource");
    glCompileShaderT = (PFNGLCOMPILESHADERPROC)getProc("glCompileShader");
    glGetShaderivT = (PFNGLGETSHADERIVPROC)getProc("glGetShaderiv");
    glGetShaderInfoLogT = (PFNGLGETSHADERINFOLOGPROC)getProc("glGetShaderInfoLog");
    glCreateProgramT = (PFNGLCREATEPROGRAMPROC)getProc("glCreateProgram");
    glAttachShaderT = (PFNGLATTACHSHADERPROC)getProc("glAttachShader");
    glLinkProgramT = (PFNGLLINKPROGRAMPROC)getProc("glLinkProgram");
    glUseProgramT = (PFNGLUSEPROGRAMPROC)getProc("glUseProgram");
    glDispatchComputeT = (PFNGLDISPATCHCOMPUTEPROC)getProc("glDispatchCompute");
    glMemoryBarrierT = (PFNGLMEMORYBARRIERPROC)getProc("glMemoryBarrier");
    glGenBuffersT = (PFNGLGENBUFFERSPROC)getProc("glGenBuffers");
    glBindBufferT = (PFNGLBINDBUFFERPROC)getProc("glBindBuffer");
    glBufferDataT = (PFNGLBUFFERDATAPROC)getProc("glBufferData");
    glBindBufferBaseT = (PFNGLBINDBUFFERBASEPROC)getProc("glBindBufferBase");
    glMapBufferT = (PFNGLMAPBUFFERPROC)getProc("glMapBuffer");
    glUnmapBufferT = (PFNGLUNMAPBUFFERPROC)getProc("glUnmapBuffer");
    glGetUniformLocationT = (PFNGLGETUNIFORMLOCATIONPROC)getProc("glGetUniformLocation");
    glUniform1iT = (PFNGLUNIFORM1IPROC)getProc("glUniform1i");
    glDeleteShaderT = (PFNGLDELETESHADERPROC)getProc("glDeleteShader");
    glDeleteProgramT = (PFNGLDELETEPROGRAMPROC)getProc("glDeleteProgram");
    glDeleteBuffersT = (PFNGLDELETEBUFFERSPROC)getProc("glDeleteBuffers");

    s_GLTInitialized = true;
}

namespace SanmapGen {

    struct LayerConfigGLSL {
        float freq;
        int octaves;
        float gain;
        int stratumIdx;
        float opacity;
        float padding[3]; // Align to vec4 layout
    };

    void TerrainCompute::DispatchTerrain(std::vector<FloatMask>& stratums, const GenerationParams& params) {
        if(params.Layers.empty()) return;

        LoadGLExtensionsT();
        if(!glCreateShaderT) {
            std::cerr << "Failed to load OpenGL compute extensions for Terrain Gen." << std::endl;
            return;
        }

        std::ifstream file("D:/Projects/Sanctuary/Map Generator/shaders/TerrainCompute.glsl");
        if(!file.is_open()) {
            std::cerr << "Failed to open TerrainCompute.glsl!" << std::endl;
            return;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string shaderSourceStr = buffer.str();
        const char* shaderSource = shaderSourceStr.c_str();

        GLuint computeShader = glCreateShaderT(GL_COMPUTE_SHADER);
        glShaderSourceT(computeShader, 1, &shaderSource, NULL);
        glCompileShaderT(computeShader);

        GLint success;
        glGetShaderivT(computeShader, 0x8B81, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLogT(computeShader, 512, NULL, infoLog);
            std::cerr << "Terrain Compute Shader Compilation Failed:\n" << infoLog << std::endl;
            return;
        }

        GLuint computeProgram = glCreateProgramT();
        glAttachShaderT(computeProgram, computeShader);
        glLinkProgramT(computeProgram);
        glDeleteShaderT(computeShader);

        std::vector<LayerConfigGLSL> activeLayers;
        for (size_t i = 0; i < params.Layers.size(); ++i) {
            const auto& layer = params.Layers[i];
            if (layer.Enabled) {
                LayerConfigGLSL cfg;
                cfg.freq = layer.Frequency;
                cfg.octaves = layer.Octaves;
                cfg.gain = layer.Gain;
                cfg.stratumIdx = static_cast<int>(i);
                cfg.opacity = layer.Opacity;
                activeLayers.push_back(cfg);
            }
        }
        
        if (activeLayers.empty()) return;
        int layerCount = (int)activeLayers.size();
        
        int totalStrataTypes = (int)params.Layers.size();
        size_t mapPixels = params.MapSize * params.MapSize;
        std::vector<float> flattenedStrata(mapPixels * totalStrataTypes, 0.0f);

        GLuint ssbo[2];
        glGenBuffersT(2, ssbo);

        glBindBufferT(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
        glBufferDataT(GL_SHADER_STORAGE_BUFFER, flattenedStrata.size() * sizeof(float), flattenedStrata.data(), GL_DYNAMIC_COPY);
        glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 0, ssbo[0]);

        glBindBufferT(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
        glBufferDataT(GL_SHADER_STORAGE_BUFFER, activeLayers.size() * sizeof(LayerConfigGLSL), activeLayers.data(), GL_DYNAMIC_COPY);
        glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1]);

        glUseProgramT(computeProgram);
        glUniform1iT(glGetUniformLocationT(computeProgram, "mapSize"), params.MapSize);
        glUniform1iT(glGetUniformLocationT(computeProgram, "layerCount"), layerCount);
        glUniform1iT(glGetUniformLocationT(computeProgram, "seed"), params.Seed);

        int workgroupX = (params.MapSize + 15) / 16;
        int workgroupY = (params.MapSize + 15) / 16;
        glDispatchComputeT(workgroupX, workgroupY, 1);
        glMemoryBarrierT(GL_SHADER_STORAGE_BARRIER_BIT);

        glBindBufferT(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
        float* ptr = (float*)glMapBufferT(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (ptr) {
            for (int s = 0; s < totalStrataTypes; ++s) {
                if (s < (int)stratums.size()) {
                    std::copy(ptr + (s * mapPixels), ptr + ((s + 1) * mapPixels), stratums[s].GetMutableDataPtr());
                }
            }
            glUnmapBufferT(GL_SHADER_STORAGE_BUFFER);
        }

        glDeleteBuffersT(2, ssbo);
        glDeleteProgramT(computeProgram);
    }
}

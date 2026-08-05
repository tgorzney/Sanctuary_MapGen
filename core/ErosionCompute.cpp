#include "ErosionCompute.h"
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

// Minimal GL extensions for Compute Shaders
typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;
#define GL_COMPUTE_SHADER                 0x91B9
#define GL_SHADER_STORAGE_BUFFER          0x90D2

typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLDISPATCHCOMPUTEPROC) (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void (APIENTRYP PFNGLMEMORYBARRIERPROC) (GLbitfield barriers);
#define GL_SHADER_STORAGE_BARRIER_BIT     0x2000
typedef void (APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
#define GL_DYNAMIC_COPY                   0x88EA
typedef void (APIENTRYP PFNGLBINDBUFFERBASEPROC) (GLenum target, GLuint index, GLuint buffer);
typedef void* (APIENTRYP PFNGLMAPBUFFERPROC) (GLenum target, GLenum access);
#define GL_READ_ONLY                      0x88B8
typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFERPROC) (GLenum target);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRYP PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);

// Function pointers
static PFNGLCREATESHADERPROC glCreateShader = nullptr;
static PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
static PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
static PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
static PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
static PFNGLATTACHSHADERPROC glAttachShader = nullptr;
static PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
static PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
static PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
static PFNGLDISPATCHCOMPUTEPROC glDispatchCompute = nullptr;
static PFNGLMEMORYBARRIERPROC glMemoryBarrier = nullptr;
static PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
static PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
static PFNGLBUFFERDATAPROC glBufferData = nullptr;
static PFNGLBINDBUFFERBASEPROC glBindBufferBase = nullptr;
static PFNGLMAPBUFFERPROC glMapBuffer = nullptr;
static PFNGLUNMAPBUFFERPROC glUnmapBuffer = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
static PFNGLUNIFORM1IPROC glUniform1i = nullptr;
static PFNGLUNIFORM1FPROC glUniform1f = nullptr;
static PFNGLDELETESHADERPROC glDeleteShader = nullptr;
static PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;

static bool s_GLInitialized = false;

static void LoadGLExtensions() {
    if (s_GLInitialized) return;
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

    glCreateShader = (PFNGLCREATESHADERPROC)getProc("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)getProc("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)getProc("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)getProc("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)getProc("glGetShaderInfoLog");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)getProc("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)getProc("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)getProc("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)getProc("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)getProc("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)getProc("glUseProgram");
    glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)getProc("glDispatchCompute");
    glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)getProc("glMemoryBarrier");
    glGenBuffers = (PFNGLGENBUFFERSPROC)getProc("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)getProc("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)getProc("glBufferData");
    glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)getProc("glBindBufferBase");
    glMapBuffer = (PFNGLMAPBUFFERPROC)getProc("glMapBuffer");
    glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)getProc("glUnmapBuffer");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)getProc("glGetUniformLocation");
    glUniform1i = (PFNGLUNIFORM1IPROC)getProc("glUniform1i");
    glUniform1f = (PFNGLUNIFORM1FPROC)getProc("glUniform1f");
    glDeleteShader = (PFNGLDELETESHADERPROC)getProc("glDeleteShader");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)getProc("glDeleteProgram");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)getProc("glDeleteBuffers");

    s_GLInitialized = true;
}

namespace SanmapGen {

    struct PhysicsData {
        float Hardness;
        float Friction;
        float Cohesion;
        float CapacityMult;
    };

    void ErosionCompute::DispatchStratified(std::vector<FloatMask>& stratums, const std::vector<DropletSpawn>& spawns, const ErosionSettings& settings, const GenerationParams& params, int mapSize, int currentLayerIdx) {
        auto flatLayers = params.GetFlatLayers();
        if(!settings.Enabled || stratums.empty() || spawns.empty() || currentLayerIdx < 0 || currentLayerIdx >= (int)flatLayers.size()) return;

        LoadGLExtensions();
        if(!glCreateShader) {
            std::cerr << "Failed to load OpenGL compute extensions for Stratified Erosion." << std::endl;
            return;
        }

        std::ifstream file("D:/Projects/Sanctuary/Map Generator/shaders/ErosionCompute.glsl");
        if(!file.is_open()) {
            std::cerr << "Failed to open ErosionCompute.glsl!" << std::endl;
            return;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string shaderSourceStr = buffer.str();
        const char* shaderSource = shaderSourceStr.c_str();

        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShader, 1, &shaderSource, NULL);
        glCompileShader(computeShader);

        GLint success;
        glGetShaderiv(computeShader, 0x8B81, &success); // GL_COMPILE_STATUS
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
            std::cerr << "Compute Shader Compilation Failed:\n" << infoLog << std::endl;
            return;
        }

        GLuint computeProgram = glCreateProgram();
        glAttachShader(computeProgram, computeShader);
        glLinkProgram(computeProgram);
        glDeleteShader(computeShader);

        // Load Avalanche Shader
        std::ifstream avaFile("D:/Projects/Sanctuary/Map Generator/shaders/AvalancheCompute.glsl");
        if(!avaFile.is_open()) {
            std::cerr << "Failed to open AvalancheCompute.glsl!" << std::endl;
            return;
        }
        std::stringstream avaBuffer;
        avaBuffer << avaFile.rdbuf();
        std::string avaSourceStr = avaBuffer.str();
        const char* avaSource = avaSourceStr.c_str();

        GLuint avaShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(avaShader, 1, &avaSource, NULL);
        glCompileShader(avaShader);

        glGetShaderiv(avaShader, 0x8B81, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLog(avaShader, 512, NULL, infoLog);
            std::cerr << "Avalanche Compute Shader Compilation Failed:\n" << infoLog << std::endl;
            return;
        }

        GLuint avaProgram = glCreateProgram();
        glAttachShader(avaProgram, avaShader);
        glLinkProgram(avaProgram);
        glDeleteShader(avaShader);

        // Gather active layers up to and including currentLayerIdx
        std::vector<size_t> activeIndices;
        for (size_t i = 0; i <= (size_t)currentLayerIdx; ++i) {
            if (flatLayers[i]->Enabled) activeIndices.push_back(i);
        }

        if(activeIndices.empty()) return;
        int layerCount = (int)activeIndices.size();

        // currentLayerSlot is the local index of currentLayerIdx inside activeIndices
        int currentLayerSlot = layerCount - 1; // it's always the last one since we scan up to it
        for (int l = 0; l < layerCount; ++l) {
            if (activeIndices[l] == (size_t)currentLayerIdx) { currentLayerSlot = l; break; }
        }

        size_t mapPixels = mapSize * mapSize;
        std::vector<float> flattenedStrata(mapPixels * layerCount, 0.0f);
        std::vector<PhysicsData> physicsArray(layerCount);

        for (int l = 0; l < layerCount; ++l) {
            size_t srcIdx = activeIndices[l];

            // Flatten thickness
            std::copy(stratums[srcIdx].GetDataPtr(), stratums[srcIdx].GetDataPtr() + mapPixels, flattenedStrata.begin() + (l * mapPixels));

            // Flatten physics — now read directly from the layer, not the stratum
            const auto& layer = (*flatLayers[srcIdx]);
            float encodedHardness = layer.Erodable ? layer.Hardness : -1.0f; // sentinel < 0 = not erodable
            physicsArray[l] = { encodedHardness, layer.Friction, layer.Cohesion, layer.CapacityMult };
        }

        // Setup SSBOs
        GLuint ssbo[3];
        glGenBuffers(3, ssbo);

        // Binding 0: Stratum thicknesses
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, flattenedStrata.size() * sizeof(float), flattenedStrata.data(), GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[0]);

        // Binding 1: Physics data
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, physicsArray.size() * sizeof(PhysicsData), physicsArray.data(), GL_DYNAMIC_COPY); // actually GL_STATIC_DRAW since read-only, but this is fine
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1]);

        // Binding 2: Droplet Spawns
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[2]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, spawns.size() * sizeof(DropletSpawn), spawns.data(), GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo[2]);

        glUseProgram(computeProgram);

        // Uniforms
        glUniform1i(glGetUniformLocation(computeProgram, "mapSize"), mapSize);
        glUniform1i(glGetUniformLocation(computeProgram, "layerCount"), layerCount);
        glUniform1i(glGetUniformLocation(computeProgram, "currentLayerSlot"), currentLayerSlot);
        glUniform1i(glGetUniformLocation(computeProgram, "maxLifetime"), settings.MaxLifetime);
        glUniform1f(glGetUniformLocation(computeProgram, "gravity"), settings.Gravity);
        glUniform1f(glGetUniformLocation(computeProgram, "evaporationRate"), settings.EvaporationRate);
        glUniform1i(glGetUniformLocation(computeProgram, "totalDroplets"), settings.DropletCount);
        glUniform1i(glGetUniformLocation(computeProgram, "depositionMode"), settings.DepositionMode ? 1 : 0);
        glUniform1i(glGetUniformLocation(computeProgram, "erodeBeneath"), (*flatLayers[currentLayerIdx]).ErodeBeneath ? 1 : 0);
        glUniform1f(glGetUniformLocation(computeProgram, "initialSedimentLoad"), settings.InitialSedimentLoad);

        int workgroupSizeX = 256;
        int numWorkgroups = (settings.DropletCount + workgroupSizeX - 1) / workgroupSizeX;
        
        glDispatchCompute(numWorkgroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        
        // Dispatch Avalanche Pass (2 iterations for stability)
        glUseProgram(avaProgram);
        glUniform1i(glGetUniformLocation(avaProgram, "mapSize"), mapSize);
        glUniform1i(glGetUniformLocation(avaProgram, "layerCount"), layerCount);
        glUniform1i(glGetUniformLocation(avaProgram, "currentLayerSlot"), currentLayerSlot);
        glUniform1i(glGetUniformLocation(avaProgram, "erodeBeneath"), (*flatLayers[currentLayerIdx]).ErodeBeneath ? 1 : 0);
        
        int avaWorkgroupX = (mapSize + 15) / 16;
        int avaWorkgroupY = (mapSize + 15) / 16;
        
        for(int p = 0; p < 2; ++p) {
            glDispatchCompute(avaWorkgroupX, avaWorkgroupY, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        // Readback — only the layers we uploaded
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
        float* ptr = (float*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (ptr) {
            for (int l = 0; l < layerCount; ++l) {
                size_t destIdx = activeIndices[l];
                std::copy(ptr + (l * mapPixels), ptr + ((l + 1) * mapPixels), stratums[destIdx].GetMutableDataPtr());

                // Clamp to prevent negative thicknesses from GPU races
                for (size_t p = 0; p < mapPixels; ++p) {
                    if (stratums[destIdx].GetMutableDataPtr()[p] < 0.0f) {
                        stratums[destIdx].GetMutableDataPtr()[p] = 0.0f;
                    }
                }
            }
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        } else {
            std::cerr << "Failed to map SSBO buffer!" << std::endl;
        }

        glDeleteBuffers(3, ssbo);
        glDeleteProgram(computeProgram);
        glDeleteProgram(avaProgram);
    }
}

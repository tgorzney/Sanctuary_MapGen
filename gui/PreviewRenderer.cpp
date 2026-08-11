#include "PreviewRenderer.h"
#include <vector>
#include <algorithm>

// GL_CLAMP_TO_EDGE is OpenGL 1.2+; define as fallback in case the bundled GL header only covers 1.1
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#define NOMINMAX
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <Windows.h>
#include <GL/gl.h>

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

namespace SanmapGen {

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;
#define GL_COMPUTE_SHADER                 0x91B9
#define GL_SHADER_STORAGE_BUFFER          0x90D2
#define GL_SHADER_STORAGE_BARRIER_BIT     0x2000
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x0020
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_RGBA8                          0x8058

typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
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
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRYP PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRYP PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRYP PFNGLUNIFORM4FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRYP PFNGLUNIFORM1FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM4FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM1IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLBINDIMAGETEXTUREPROC) (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
typedef void (APIENTRYP PFNGLGETBUFFERSUBDATAPROC) (GLenum target, GLintptr offset, GLsizeiptr size, void *data);
typedef void (APIENTRYP PFNGLBUFFERSUBDATAPROC) (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void* (APIENTRYP PFNGLMAPBUFFERRANGEPROC) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFERPROC) (GLenum target);

#define GL_MAP_WRITE_BIT 0x0002
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020

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
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocationT = nullptr;
static PFNGLUNIFORM1IPROC glUniform1iT = nullptr;
static PFNGLUNIFORM1FPROC glUniform1fT = nullptr;
static PFNGLUNIFORM2FPROC glUniform2fT = nullptr;
static PFNGLUNIFORM4FPROC glUniform4fT = nullptr;
static PFNGLUNIFORM1FVPROC glUniform1fvT = nullptr;
static PFNGLUNIFORM4FVPROC glUniform4fvT = nullptr;
static PFNGLUNIFORM1IVPROC glUniform1ivT = nullptr;
static PFNGLDELETESHADERPROC glDeleteShaderT = nullptr;
static PFNGLDELETEPROGRAMPROC glDeleteProgramT = nullptr;
static PFNGLBINDIMAGETEXTUREPROC glBindImageTextureT = nullptr;
static PFNGLGETBUFFERSUBDATAPROC glGetBufferSubDataT = nullptr;
static PFNGLBUFFERSUBDATAPROC glBufferSubDataT = nullptr;
static PFNGLMAPBUFFERRANGEPROC glMapBufferRangeT = nullptr;
static PFNGLUNMAPBUFFERPROC glUnmapBufferT = nullptr;
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
    glGetUniformLocationT = (PFNGLGETUNIFORMLOCATIONPROC)getProc("glGetUniformLocation");
    glUniform1iT = (PFNGLUNIFORM1IPROC)getProc("glUniform1i");
    glUniform1fT = (PFNGLUNIFORM1FPROC)getProc("glUniform1f");
    glUniform2fT = (PFNGLUNIFORM2FPROC)getProc("glUniform2f");
    glUniform4fT = (PFNGLUNIFORM4FPROC)getProc("glUniform4f");
    glUniform1fvT = (PFNGLUNIFORM1FVPROC)getProc("glUniform1fv");
    glUniform4fvT = (PFNGLUNIFORM4FVPROC)getProc("glUniform4fv");
    glUniform1ivT = (PFNGLUNIFORM1IVPROC)getProc("glUniform1iv");
    glDeleteShaderT = (PFNGLDELETESHADERPROC)getProc("glDeleteShader");
    glDeleteProgramT = (PFNGLDELETEPROGRAMPROC)getProc("glDeleteProgram");
    glBindImageTextureT = (PFNGLBINDIMAGETEXTUREPROC)getProc("glBindImageTexture");
    glGetBufferSubDataT = (PFNGLGETBUFFERSUBDATAPROC)getProc("glGetBufferSubData");
    glBufferSubDataT = (PFNGLBUFFERSUBDATAPROC)getProc("glBufferSubData");
    glMapBufferRangeT = (PFNGLMAPBUFFERRANGEPROC)getProc("glMapBufferRange");
    glUnmapBufferT = (PFNGLUNMAPBUFFERPROC)getProc("glUnmapBuffer");
    s_GLTInitialized = true;
}

    struct ShaderPermutation {
        GLuint Program;
        GLint loc_width, loc_height, loc_quadWidth, loc_quadHeight;
        GLint loc_cellSize, loc_bUseEngineParityMath, loc_minHeight, loc_maxHeight, loc_autoLevelPreview;
        GLint loc_currentLayerBlend, loc_numStratums, loc_stratumColors, loc_stratumRemaps;
        GLint loc_flowMapColor, loc_waterLevelMax, loc_deepWaterMin, loc_deepWaterMax, loc_terrainMinHeight;
        GLint loc_numAreas, loc_numRules;
        GLint loc_focusDebugRuleIndex, loc_focusGradientType, loc_focusGradientRadius, loc_focusGradientContrast, loc_focusGradientStrength;
        
        void Init(GLuint prog) {
            Program = prog;
            loc_width = glGetUniformLocationT(prog, "width");
            loc_height = glGetUniformLocationT(prog, "height");
            loc_quadWidth = glGetUniformLocationT(prog, "quadWidth");
            loc_quadHeight = glGetUniformLocationT(prog, "quadHeight");
            loc_cellSize = glGetUniformLocationT(prog, "cellSize");
            loc_bUseEngineParityMath = glGetUniformLocationT(prog, "bUseEngineParityMath");
            loc_minHeight = glGetUniformLocationT(prog, "minHeight");
            loc_maxHeight = glGetUniformLocationT(prog, "maxHeight");
            loc_autoLevelPreview = glGetUniformLocationT(prog, "autoLevelPreview");
            loc_currentLayerBlend = glGetUniformLocationT(prog, "currentLayerBlend");
            loc_numStratums = glGetUniformLocationT(prog, "numStratums");
            loc_stratumColors = glGetUniformLocationT(prog, "stratumColors");
            loc_stratumRemaps = glGetUniformLocationT(prog, "stratumRemaps");
            loc_flowMapColor = glGetUniformLocationT(prog, "flowMapColor");
            loc_waterLevelMax = glGetUniformLocationT(prog, "waterLevelMax");
            loc_deepWaterMin = glGetUniformLocationT(prog, "deepWaterMin");
            loc_deepWaterMax = glGetUniformLocationT(prog, "deepWaterMax");
            loc_terrainMinHeight = glGetUniformLocationT(prog, "terrainMinHeight");
            loc_numAreas = glGetUniformLocationT(prog, "numAreas");
            loc_numRules = glGetUniformLocationT(prog, "numRules");
            loc_focusDebugRuleIndex = glGetUniformLocationT(prog, "focusDebugRuleIndex");
            loc_focusGradientType = glGetUniformLocationT(prog, "focusGradientType");
            loc_focusGradientRadius = glGetUniformLocationT(prog, "focusGradientRadius");
            loc_focusGradientContrast = glGetUniformLocationT(prog, "focusGradientContrast");
            loc_focusGradientStrength = glGetUniformLocationT(prog, "focusGradientStrength");
        }
    };

static ShaderPermutation s_ComputePrograms[15];
static GLuint s_SSBOs[8] = {0};
static bool s_ShaderInitialized = false;
static int s_AllocatedWidth = 0;
static int s_AllocatedHeight = 0;

static void InitializeShader() {
    LoadGLExtensionsT();
    if (!glCreateShaderT) return;
    std::ifstream file("D:/Projects/Sanctuary/Map Generator/shaders/PreviewCompute.glsl");
    if(!file.is_open()) return;
    std::stringstream buffer; buffer << file.rdbuf();
    std::string baseSourceStr = buffer.str();

    PFNGLGETPROGRAMIVPROC glGetProgramivT = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    if(!glGetProgramivT) glGetProgramivT = (PFNGLGETPROGRAMIVPROC)GetProcAddress(GetModuleHandleA("opengl32.dll"), "glGetProgramiv");
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLogT = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    if(!glGetProgramInfoLogT) glGetProgramInfoLogT = (PFNGLGETPROGRAMINFOLOGPROC)GetProcAddress(GetModuleHandleA("opengl32.dll"), "glGetProgramInfoLog");

    auto compilePermutation = [&](const std::string& defineMacros) -> GLuint {
        size_t versionEnd = baseSourceStr.find('\n');
        if (versionEnd == std::string::npos) versionEnd = 16;
        std::string sourceStr = baseSourceStr.substr(0, versionEnd + 1) + defineMacros + "\n" + baseSourceStr.substr(versionEnd + 1);
        const char* source = sourceStr.c_str();
        GLuint cs = glCreateShaderT(GL_COMPUTE_SHADER);
        glShaderSourceT(cs, 1, &source, NULL);
        glCompileShaderT(cs);
        GLint success;
        glGetShaderivT(cs, 0x8B81, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLogT(cs, 512, NULL, infoLog);
            std::cerr << "Preview Compute Shader Compilation Failed (" << defineMacros << "):\n" << infoLog << std::endl;
            return 0;
        }
        GLuint prog = glCreateProgramT();
        glAttachShaderT(prog, cs);
        glLinkProgramT(prog);
        
        if (glGetProgramivT && glGetProgramInfoLogT) {
            GLint linkSuccess;
            glGetProgramivT(prog, 0x8B82 /* GL_LINK_STATUS */, &linkSuccess);
            if (!linkSuccess) {
                GLchar infoLog[512];
                glGetProgramInfoLogT(prog, 512, NULL, infoLog);
                std::cerr << "Preview Compute Shader Link Failed (" << defineMacros << "):\n" << infoLog << std::endl;
            }
        }
        glDeleteShaderT(cs);
        return prog;
    };

    s_ComputePrograms[0].Init(compilePermutation("#define PASS_CLEAR"));
    for (int i = 0; i < 13; ++i) {
        s_ComputePrograms[i + 1].Init(compilePermutation("#define PASS_LAYER_" + std::to_string(i)));
    }
    s_ComputePrograms[14].Init(compilePermutation("#define PASS_OVERLAY"));

    s_ShaderInitialized = true;
}



    GLuint PreviewRenderer::UpdatePreviewTexture(const FloatMask& heightmap, const GenerationResult& genResult, const GenerationParams& params, GLuint existingTexture, bool bGeometryChanged) {
        int width = heightmap.GetWidth();
        int height = heightmap.GetHeight();

        // If the mask is empty, return 0
        if (width <= 1 || height <= 1) return existingTexture;

        int quadWidth = width - 1;
        int quadHeight = height - 1;
        
                // Ensure GL extensions and Compute Shader are initialized
        if (!s_ShaderInitialized) {
            InitializeShader();
        }

        float minHeight = 0.0f;
        float maxHeight = 1.0f;
        if (params.AutoLevelPreview) {
            minHeight = 1e10f;
            maxHeight = -1e10f;
            for (int i = 0; i < width * height; ++i) {
                float h = heightmap.GetDataPtr()[i];
                if (h < minHeight) minHeight = h;
                if (h > maxHeight) maxHeight = h;
            }
            if (maxHeight - minHeight < 0.0001f) {
                minHeight = 0.0f;
                maxHeight = 1.0f;
            }
        }

        params.EntityIDBuffer.assign(quadWidth * quadHeight, 0xFFFFFFFF);
        params.EntityIDBufferWidth = quadWidth;
        params.EntityIDBufferHeight = quadHeight;

        GLuint textureID = existingTexture;
        if (s_SSBOs[0] == 0) {
            glGenBuffersT(8, s_SSBOs);
        }
        if (textureID == 0) {
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        bool bNeedsReallocation = (s_AllocatedWidth != quadWidth || s_AllocatedHeight != quadHeight);
        if (bNeedsReallocation) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, quadWidth, quadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, 0);
            
            s_AllocatedWidth = quadWidth;
            s_AllocatedHeight = quadHeight;
            
            if (s_SSBOs[0] != 0) {
                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[0]);
                glBufferDataT(GL_SHADER_STORAGE_BUFFER, quadWidth * quadHeight * sizeof(uint32_t), nullptr, GL_DYNAMIC_COPY);
                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[1]);
                glBufferDataT(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), nullptr, GL_DYNAMIC_COPY);
                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[2]);
                glBufferDataT(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), nullptr, GL_DYNAMIC_COPY);
                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[3]);
                glBufferDataT(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), nullptr, GL_DYNAMIC_COPY);
                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[4]);
                glBufferDataT(GL_SHADER_STORAGE_BUFFER, width * height * 9 * sizeof(float), nullptr, GL_DYNAMIC_COPY);
            }
        }

        if (s_ShaderInitialized) {
            // Ensure base bindings
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 0, s_SSBOs[0]);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 1, s_SSBOs[1]);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 2, s_SSBOs[2]);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 3, s_SSBOs[3]);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 4, s_SSBOs[4]);

            if (bGeometryChanged || bNeedsReallocation) {
                // Upload data to SSBOs
                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[0]);
                glBufferSubDataT(GL_SHADER_STORAGE_BUFFER, 0, params.EntityIDBuffer.size() * sizeof(uint32_t), params.EntityIDBuffer.data());
                
                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[1]);
                glBufferSubDataT(GL_SHADER_STORAGE_BUFFER, 0, width * height * sizeof(float), heightmap.GetDataPtr());

                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[2]);
                glBufferSubDataT(GL_SHADER_STORAGE_BUFFER, 0, width * height * sizeof(float), genResult.FlowMap.GetDataPtr());

                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[3]);
                glBufferSubDataT(GL_SHADER_STORAGE_BUFFER, 0, width * height * sizeof(float), genResult.AccumulationMap.GetDataPtr());
                
                // Material Masks (stratums)
                std::vector<float> stratumsFlat(width * height * 9, 0.0f);
                for(size_t i = 0; i < genResult.MaterialMasks.size() && i < 9; ++i) {
                    memcpy(stratumsFlat.data() + i * (width * height), genResult.MaterialMasks[i].GetDataPtr(), width * height * sizeof(float));
                }
                glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[4]);
                glBufferSubDataT(GL_SHADER_STORAGE_BUFFER, 0, stratumsFlat.size() * sizeof(float), stratumsFlat.data());
            } // end bGeometryChanged

            // Areas
            std::vector<float> areaBoundsFlat;
            std::vector<float> areaColorsFlat;
            for(const auto& area : params.Areas) {
                areaBoundsFlat.push_back(area.X); areaBoundsFlat.push_back(area.Y);
                areaBoundsFlat.push_back(area.Width); areaBoundsFlat.push_back(area.Length);
                areaColorsFlat.push_back(area.Color[0]); areaColorsFlat.push_back(area.Color[1]);
                areaColorsFlat.push_back(area.Color[2]); areaColorsFlat.push_back(area.Color[3]);
            }
            if(areaBoundsFlat.empty()) { areaBoundsFlat.resize(4, 0.0f); areaColorsFlat.resize(4, 0.0f); }
            
            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[5]);
            size_t areaBoundsSize = areaBoundsFlat.size() * sizeof(float);
            size_t areaColorsSize = areaColorsFlat.size() * sizeof(float);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, areaBoundsSize + areaColorsSize, nullptr, GL_DYNAMIC_DRAW);
            void* ptrAreas = glMapBufferRangeT(GL_SHADER_STORAGE_BUFFER, 0, areaBoundsSize + areaColorsSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
            if (ptrAreas) {
                std::memcpy(ptrAreas, areaBoundsFlat.data(), areaBoundsSize);
                std::memcpy(static_cast<char*>(ptrAreas) + areaBoundsSize, areaColorsFlat.data(), areaColorsSize);
                glUnmapBufferT(GL_SHADER_STORAGE_BUFFER);
            }
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 5, s_SSBOs[5]);

            // Rules for Markers / Props
            std::vector<float> ruleBoundsFlat;
            std::vector<float> ruleParamsFlat; // density, isMarker, isProp, ruleIndex
            if (!params.ProceduralMarkerLayers.empty() && params.ProceduralMarkerLayers[0].Enabled) {
                for (const auto& rule : params.ProceduralMarkerLayers[0].Rules) {
                    if(!rule.Enabled) continue;
                    ruleBoundsFlat.push_back(rule.MinSlope); ruleBoundsFlat.push_back(rule.MaxSlope);
                    ruleBoundsFlat.push_back(rule.MinHeight); ruleBoundsFlat.push_back(rule.MaxHeight);
                    ruleParamsFlat.push_back(rule.Density); ruleParamsFlat.push_back(1.0f); ruleParamsFlat.push_back(0.0f); ruleParamsFlat.push_back(0.0f);
                }
            }
            for (const auto& gl : params.GeoLayers) {
                if (gl.Type != LayerType::Prop || !gl.Enabled) continue;
                for (const auto& rule : gl.Layers) {
                    if(!rule.Enabled) continue;
                    ruleBoundsFlat.push_back(rule.MinSlope); ruleBoundsFlat.push_back(rule.MaxSlope);
                    ruleBoundsFlat.push_back(rule.MinHeight); ruleBoundsFlat.push_back(rule.MaxHeight);
                    ruleParamsFlat.push_back(rule.LandDensity); ruleParamsFlat.push_back(0.0f); ruleParamsFlat.push_back(1.0f); ruleParamsFlat.push_back(0.0f);
                }
            }
            if(ruleBoundsFlat.empty()) { ruleBoundsFlat.resize(4, 0.0f); ruleParamsFlat.resize(4, 0.0f); }

            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[6]);
            size_t boundsSize = ruleBoundsFlat.size() * sizeof(float);
            size_t paramsSize = ruleParamsFlat.size() * sizeof(float);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, boundsSize + paramsSize, nullptr, GL_DYNAMIC_DRAW);
            void* ptrRules = glMapBufferRangeT(GL_SHADER_STORAGE_BUFFER, 0, boundsSize + paramsSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
            if (ptrRules) {
                std::memcpy(ptrRules, ruleBoundsFlat.data(), boundsSize);
                std::memcpy(static_cast<char*>(ptrRules) + boundsSize, ruleParamsFlat.data(), paramsSize);
                glUnmapBufferT(GL_SHADER_STORAGE_BUFFER);
            }
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 6, s_SSBOs[6]);

            // Bind Texture as Image
            glBindImageTextureT(0, textureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

            int numStratums = (int)genResult.MaterialMasks.size();
            std::vector<float> stratColors, stratRemaps;
            for(int i=0; i<9; ++i) {
                if(i < (int)params.Stratums.size()) {
                    stratColors.push_back(params.Stratums[i].previewColor[0]);
                    stratColors.push_back(params.Stratums[i].previewColor[1]);
                    stratColors.push_back(params.Stratums[i].previewColor[2]);
                    stratColors.push_back(1.0f);
                    stratRemaps.push_back(params.Stratums[i].maskRemapMin[0]);
                    stratRemaps.push_back(params.Stratums[i].maskRemapMax[0]);
                } else {
                    for(int c=0; c<4; ++c) stratColors.push_back(0.0f);
                    stratRemaps.push_back(0.0f); stratRemaps.push_back(1.0f);
                }
            }
            
            // Build baked gradient caches
            auto buildGradientCache = [](const GradientSettings& settings, float* cache) {
                for (int i = 0; i < 256; ++i) {
                    float val = i / 255.0f;
                    float outR = 0, outG = 0, outB = 0, outA = 0;
                    const auto& stops = settings.Stops;
                    if (stops.empty()) { cache[i*4+0] = 1; cache[i*4+1] = 1; cache[i*4+2] = 1; cache[i*4+3] = 1; continue; }
                    if (val <= stops.front().Location) {
                        outR = stops.front().Color[0]; outG = stops.front().Color[1]; outB = stops.front().Color[2]; outA = stops.front().Color[3];
                    } else if (val >= stops.back().Location) {
                        outR = stops.back().Color[0]; outG = stops.back().Color[1]; outB = stops.back().Color[2]; outA = stops.back().Color[3];
                    } else {
                        for (size_t j = 0; j < stops.size() - 1; ++j) {
                            if (val >= stops[j].Location && val <= stops[j+1].Location) {
                                if (settings.SmoothInterpolation) {
                                    float range = stops[j+1].Location - stops[j].Location;
                                    float t = (val - stops[j].Location) / std::max(0.001f, range);
                                    outR = stops[j].Color[0] * (1.0f - t) + stops[j+1].Color[0] * t;
                                    outG = stops[j].Color[1] * (1.0f - t) + stops[j+1].Color[1] * t;
                                    outB = stops[j].Color[2] * (1.0f - t) + stops[j+1].Color[2] * t;
                                    outA = stops[j].Color[3] * (1.0f - t) + stops[j+1].Color[3] * t;
                                } else {
                                    outR = stops[j].Color[0]; outG = stops[j].Color[1]; outB = stops[j].Color[2]; outA = stops[j].Color[3];
                                }
                                break;
                            }
                        }
                    }
                    cache[i*4+0] = outR; cache[i*4+1] = outG; cache[i*4+2] = outB; cache[i*4+3] = outA;
                }
            };
            float slopeGrad[256*4], flowGrad[256*4], accumGrad[256*4], waterGrad[256*4];
            buildGradientCache(params.SlopeSettingsParams.Gradient, slopeGrad);
            buildGradientCache(params.FlowSettingsParams.Gradient, flowGrad);
            buildGradientCache(params.FlowSettingsParams.Gradient, accumGrad); // Same for accumulation in legacy
            buildGradientCache(params.Water.Gradient, waterGrad);
            
            std::vector<float> gradientCacheFlat;
            gradientCacheFlat.reserve(256 * 4 * 4);
            gradientCacheFlat.insert(gradientCacheFlat.end(), slopeGrad, slopeGrad + 256*4);
            gradientCacheFlat.insert(gradientCacheFlat.end(), flowGrad, flowGrad + 256*4);
            gradientCacheFlat.insert(gradientCacheFlat.end(), accumGrad, accumGrad + 256*4);
            gradientCacheFlat.insert(gradientCacheFlat.end(), waterGrad, waterGrad + 256*4);

            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[7]);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, gradientCacheFlat.size() * sizeof(float), gradientCacheFlat.data(), GL_DYNAMIC_COPY);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 7, s_SSBOs[7]);

            GLuint numGroupsX = (quadWidth + 15) / 16;
            GLuint numGroupsY = (quadHeight + 15) / 16;
            float cellSize = static_cast<float>(params.MapSize) / quadWidth;
            if (cellSize < 1.0f) cellSize = 1.0f;

            auto dispatchProgram = [&](int permIdx, int currentLayerBlend) {
                const auto& p = s_ComputePrograms[permIdx];
                if(p.Program == 0) return;
                glUseProgramT(p.Program);

                glUniform1iT(p.loc_width, width);
                glUniform1iT(p.loc_height, height);
                glUniform1iT(p.loc_quadWidth, quadWidth);
                glUniform1iT(p.loc_quadHeight, quadHeight);
                glUniform1fT(p.loc_cellSize, cellSize);
                glUniform1iT(p.loc_bUseEngineParityMath, params.SlopeSettingsParams.bUseEngineParityMath ? 1 : 0);
                glUniform1fT(p.loc_minHeight, minHeight);
                glUniform1fT(p.loc_maxHeight, maxHeight);
                glUniform1iT(p.loc_autoLevelPreview, params.AutoLevelPreview ? 1 : 0);
                glUniform1iT(p.loc_currentLayerBlend, currentLayerBlend);
                
                glUniform1iT(p.loc_numStratums, numStratums);
                if (p.loc_stratumColors != -1) glUniform4fvT(p.loc_stratumColors, 9, stratColors.data());
                if (p.loc_stratumRemaps != -1) glUniform2fT(p.loc_stratumRemaps, stratRemaps[0], stratRemaps[1]);
                
                if (p.loc_flowMapColor != -1) glUniform4fT(p.loc_flowMapColor, params.FlowMapColor[0], params.FlowMapColor[1], params.FlowMapColor[2], params.FlowMapColor[3]);
                glUniform1fT(p.loc_waterLevelMax, params.Water.WaterLevelMax);
                glUniform1fT(p.loc_deepWaterMin, params.Water.DeepWaterDepthMin);
                glUniform1fT(p.loc_deepWaterMax, params.Water.DeepWaterDepthMax);
                glUniform1fT(p.loc_terrainMinHeight, genResult.TerrainMinHeight);
                
                glUniform1iT(p.loc_numAreas, (int)params.Areas.size());
                glUniform1iT(p.loc_numRules, (int)ruleBoundsFlat.size()/4);
                
                glUniform1iT(p.loc_focusDebugRuleIndex, params.ShowFocusGradientDebugRuleIndex);
                if(params.ShowFocusGradientDebugRuleIndex >= 0 && params.ShowFocusGradientDebugRuleIndex < (int)params.ProceduralMarkerLayers[0].Rules.size()) {
                    auto& r = params.ProceduralMarkerLayers[0].Rules[params.ShowFocusGradientDebugRuleIndex];
                    glUniform1iT(p.loc_focusGradientType, r.FocusGradient);
                    glUniform1fT(p.loc_focusGradientRadius, r.FocusGradientRadius);
                    glUniform1fT(p.loc_focusGradientContrast, r.FocusGradientContrast);
                    glUniform1fT(p.loc_focusGradientStrength, r.FocusGradientStrength);
                }

                glDispatchComputeT(numGroupsX, numGroupsY, 1);
                glMemoryBarrierT(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
            };

            // 1. Dispatch Clear Pass (idx 0)
            dispatchProgram(0, -1);

            // 2. Dispatch Active Layers in precise UI render order
            for (const auto& layer : params.PreviewLayers) {
                if (layer.Enabled && layer.Blend != GenerationParams::LayerBlendMode::None) {
                    int layerType = (int)layer.Type;
                    int currentLayerBlend = (int)layer.Blend - 1; // mapping None=0 -> -1
                    dispatchProgram(layerType + 1, currentLayerBlend);
                }
            }

            // 3. Dispatch Overlay Pass (idx 14)
            if (params.ShowFocusGradientDebugRuleIndex >= 0) {
                dispatchProgram(14, -1);
            }
            
            // Read back EntityIDBuffer
            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[0]);
            glGetBufferSubDataT(GL_SHADER_STORAGE_BUFFER, 0, params.EntityIDBuffer.size() * sizeof(uint32_t), params.EntityIDBuffer.data());
            
            glUseProgramT(0);
        }
        return textureID;
    }

} // namespace SanmapGen

import re

with open("D:/Projects/Sanctuary/Map Generator/gui/PreviewRenderer.cpp", "r") as f:
    content = f.read()

gl_ext = """
#include <iostream>
#include <fstream>
#include <sstream>
#define NOMINMAX
#include <Windows.h>
#include <GL/gl.h>

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;
#define GL_COMPUTE_SHADER                 0x91B9
#define GL_SHADER_STORAGE_BUFFER          0x90D2
#define GL_SHADER_STORAGE_BARRIER_BIT     0x2000
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x0020
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_RGBA8                          0x8058

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
    s_GLTInitialized = true;
}

static GLuint s_ComputeProgram = 0;
static GLuint s_SSBOs[8] = {0};
static bool s_ShaderInitialized = false;

static void InitializeShader() {
    LoadGLExtensionsT();
    if (!glCreateShaderT) return;
    std::ifstream file("D:/Projects/Sanctuary/Map Generator/shaders/PreviewCompute.glsl");
    if(!file.is_open()) return;
    std::stringstream buffer; buffer << file.rdbuf();
    std::string sourceStr = buffer.str();
    const char* source = sourceStr.c_str();
    GLuint cs = glCreateShaderT(GL_COMPUTE_SHADER);
    glShaderSourceT(cs, 1, &source, NULL);
    glCompileShaderT(cs);
    GLint success;
    glGetShaderivT(cs, 0x8B81, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLogT(cs, 512, NULL, infoLog);
        std::cerr << "Preview Compute Shader Compilation Failed:\\n" << infoLog << std::endl;
        return;
    }
    s_ComputeProgram = glCreateProgramT();
    glAttachShaderT(s_ComputeProgram, cs);
    glLinkProgramT(s_ComputeProgram);
    glDeleteShaderT(cs);
    glGenBuffersT(8, s_SSBOs);
    s_ShaderInitialized = true;
}
"""

replacement = """        // Ensure GL extensions and Compute Shader are initialized
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
        if (textureID == 0) {
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, quadWidth, quadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        if (s_ShaderInitialized) {
            // Upload data to SSBOs
            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[0]);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, params.EntityIDBuffer.size() * sizeof(uint32_t), params.EntityIDBuffer.data(), GL_DYNAMIC_COPY);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 1, s_SSBOs[0]);
            
            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[1]);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), heightmap.GetDataPtr(), GL_DYNAMIC_COPY);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 2, s_SSBOs[1]);

            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[2]);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), genResult.FlowMap.GetDataPtr(), GL_DYNAMIC_COPY);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 3, s_SSBOs[2]);

            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[3]);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), genResult.AccumulationMap.GetDataPtr(), GL_DYNAMIC_COPY);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 4, s_SSBOs[3]);
            
            // Material Masks (stratums)
            std::vector<float> stratumsFlat(width * height * 9, 0.0f);
            for(size_t i = 0; i < genResult.MaterialMasks.size() && i < 9; ++i) {
                memcpy(stratumsFlat.data() + i * (width * height), genResult.MaterialMasks[i].GetDataPtr(), width * height * sizeof(float));
            }
            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[4]);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, stratumsFlat.size() * sizeof(float), stratumsFlat.data(), GL_DYNAMIC_COPY);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 5, s_SSBOs[4]);

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
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, areaBoundsFlat.size() * sizeof(float), areaBoundsFlat.data(), GL_DYNAMIC_COPY);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 6, s_SSBOs[5]);
            
            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[6]);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, areaColorsFlat.size() * sizeof(float), areaColorsFlat.data(), GL_DYNAMIC_COPY);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 7, s_SSBOs[6]);

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

            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[7]);
            glBufferDataT(GL_SHADER_STORAGE_BUFFER, ruleBoundsFlat.size() * sizeof(float), ruleBoundsFlat.data(), GL_DYNAMIC_COPY);
            glBindBufferBaseT(GL_SHADER_STORAGE_BUFFER, 8, s_SSBOs[7]);

            glUseProgramT(s_ComputeProgram);

            // Bind Texture as Image
            glBindImageTextureT(0, textureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

            // Setup Uniforms
            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "width"), width);
            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "height"), height);
            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "quadWidth"), quadWidth);
            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "quadHeight"), quadHeight);
            
            float cellSize = static_cast<float>(params.MapSize) / quadWidth;
            if (cellSize < 1.0f) cellSize = 1.0f;
            glUniform1fT(glGetUniformLocationT(s_ComputeProgram, "cellSize"), cellSize);
            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "bUseEngineParityMath"), params.SlopeSettingsParams.bUseEngineParityMath ? 1 : 0);
            glUniform1fT(glGetUniformLocationT(s_ComputeProgram, "minHeight"), minHeight);
            glUniform1fT(glGetUniformLocationT(s_ComputeProgram, "maxHeight"), maxHeight);
            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "autoLevelPreview"), params.AutoLevelPreview ? 1 : 0);
            
            int layerBlends[13];
            for(int i=0; i<13; ++i) layerBlends[i] = -1;
            for (const auto& layer : params.PreviewLayers) {
                if(layer.Blend != GenerationParams::LayerBlendMode::None) {
                    layerBlends[(int)layer.Type] = (int)layer.Blend - 1; // mapping None=0 -> -1
                }
            }
            glUniform1ivT(glGetUniformLocationT(s_ComputeProgram, "layerBlends"), 13, layerBlends);

            int numStratums = (int)genResult.MaterialMasks.size();
            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "numStratums"), numStratums);
            
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
            glUniform4fvT(glGetUniformLocationT(s_ComputeProgram, "stratumColors"), 9, stratColors.data());
            glUniform2fT(glGetUniformLocationT(s_ComputeProgram, "stratumRemaps"), stratRemaps[0], stratRemaps[1]);
            
            glUniform4fT(glGetUniformLocationT(s_ComputeProgram, "flowMapColor"), params.FlowMapColor[0], params.FlowMapColor[1], params.FlowMapColor[2], params.FlowMapColor[3]);
            glUniform1fT(glGetUniformLocationT(s_ComputeProgram, "waterLevelMax"), params.Water.WaterLevelMax);
            glUniform1fT(glGetUniformLocationT(s_ComputeProgram, "terrainMinHeight"), genResult.TerrainMinHeight);
            
            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "numAreas"), (int)params.Areas.size());
            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "numRules"), (int)ruleBoundsFlat.size()/4);
            
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
            
            glUniform4fvT(glGetUniformLocationT(s_ComputeProgram, "slopeGradient"), 256, slopeGrad);
            glUniform4fvT(glGetUniformLocationT(s_ComputeProgram, "flowGradient"), 256, flowGrad);
            glUniform4fvT(glGetUniformLocationT(s_ComputeProgram, "accumGradient"), 256, accumGrad);
            glUniform4fvT(glGetUniformLocationT(s_ComputeProgram, "waterGradient"), 256, waterGrad);

            glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "focusDebugRuleIndex"), params.ShowFocusGradientDebugRuleIndex);
            if(params.ShowFocusGradientDebugRuleIndex >= 0 && params.ShowFocusGradientDebugRuleIndex < (int)params.ProceduralMarkerLayers[0].Rules.size()) {
                auto& r = params.ProceduralMarkerLayers[0].Rules[params.ShowFocusGradientDebugRuleIndex];
                glUniform1iT(glGetUniformLocationT(s_ComputeProgram, "focusGradientType"), r.FocusGradient);
                glUniform1fT(glGetUniformLocationT(s_ComputeProgram, "focusGradientRadius"), r.FocusGradientRadius);
                glUniform1fT(glGetUniformLocationT(s_ComputeProgram, "focusGradientContrast"), r.FocusGradientContrast);
                glUniform1fT(glGetUniformLocationT(s_ComputeProgram, "focusGradientStrength"), r.FocusGradientStrength);
            }

            // Dispatch Compute
            GLuint numGroupsX = (quadWidth + 15) / 16;
            GLuint numGroupsY = (quadHeight + 15) / 16;
            glDispatchComputeT(numGroupsX, numGroupsY, 1);
            
            // Memory barrier for image and SSBO access
            glMemoryBarrierT(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
            
            // Read back EntityIDBuffer
            glBindBufferT(GL_SHADER_STORAGE_BUFFER, s_SSBOs[0]);
            glGetBufferSubDataT(GL_SHADER_STORAGE_BUFFER, 0, params.EntityIDBuffer.size() * sizeof(uint32_t), params.EntityIDBuffer.data());
            
            glUseProgramT(0);
        }
"""

start_marker = "static std::vector<float> cachedSlopes;"
end_marker = "        // Upload the pixel buffer to the GPU"

start_idx = content.find(start_marker)
end_idx = content.find(end_marker)

if start_idx == -1 or end_idx == -1:
    print("Could not find markers to replace")
else:
    new_content = content[:content.find("namespace SanmapGen {") + len("namespace SanmapGen {")] + "\n" + gl_ext + "\n" + content[content.find("namespace SanmapGen {") + len("namespace SanmapGen {"):start_idx] + replacement + content[end_idx:]

    with open("D:/Projects/Sanctuary/Map Generator/gui/PreviewRenderer.cpp", "w") as f:
        f.write(new_content)
    print("Successfully patched PreviewRenderer.cpp")


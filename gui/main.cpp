#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <string>
#include <math.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include "Parameters.h"
#include "PreviewRenderer.h"
#include "TerrainGenerator.h"
#include "MapExporter.h"
#include "FileDialog.h"
#include "stb_image.h"
#include "stb_image_write.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 4.3 + GLSL 430 (Required for Compute Shaders)
    const char* glsl_version = "#version 430";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Sanctuary Map Generator", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style (Premium Dark Theme)
    ImGui::StyleColorsDark();
    
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Variables for the Map Generator UI
    SanmapGen::GenerationParams params;
    GLuint previewTexture = 0; // OpenGL texture ID for the map preview

    // Main loop
    bool bNeedsPreviewUpdate = true; // Set true initially to generate a map on startup
    
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // We enable Docking above, so you can drag these windows anywhere!
        
        // --- WINDOW 1: SETTINGS ---
        ImGui::SetNextWindowSize(ImVec2(450, 700), ImGuiCond_FirstUseEver);
        ImGui::Begin("Generator Settings");
        
        if (ImGui::BeginTabBar("GeneratorTabs"))
        {
            if (ImGui::BeginTabItem("Heightmap"))
            {
                ImGui::Checkbox("##showHeightmap", &params.ShowHeightmap); ImGui::SameLine();
                ImGui::Text("Global Settings");
                ImGui::Separator();
                if (ImGui::Checkbox("Use GPU Terrain Generation (GLSL)", &params.UseGPUTerrain)) bNeedsPreviewUpdate = true;
                if (ImGui::InputInt("Map Seed", &params.Seed)) bNeedsPreviewUpdate = true;
                
                const int sizes[] = { 256, 512, 1024, 2048, 4096 };
                const char* size_labels[] = { "256", "512", "1024", "2048", "4096" };
                static int size_current_idx = 1; // Default 512
                static bool bScaleWithMapSize = true;
                
                ImGui::Checkbox("Scale features to Map Size", &bScaleWithMapSize);
                if (ImGui::BeginCombo("Map Size", size_labels[size_current_idx])) {
                    for (int n = 0; n < IM_ARRAYSIZE(sizes); n++) {
                        const bool is_selected = (size_current_idx == n);
                        if (ImGui::Selectable(size_labels[n], is_selected)) {
                            if (size_current_idx != n) {
                                int oldSize = sizes[size_current_idx];
                                int newSize = sizes[n];
                                size_current_idx = n;
                                params.MapSize = sizes[size_current_idx];
                                bNeedsPreviewUpdate = true;
                                
                                if (bScaleWithMapSize) {
                                    float scaleFactor = static_cast<float>(oldSize) / static_cast<float>(newSize);
                                    for (auto& layer : params.Layers) {
                                        layer.Frequency *= scaleFactor;
                                    }
                                }
                            }
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                params.MapSize = sizes[size_current_idx];
                
                ImGui::Spacing();
                ImGui::Text("Symmetry Blending Mode");
                ImGui::Separator();
                
                const char* alg_labels[] = { "2D Fold (Hardlines)", "2D Blur (Post-Process)", "2D Cross-Fade", "3D Cylinder", "3D Torus", "Native Hash (X/Z/Point)", "Superposition (Multipass)" };
                int current_alg = static_cast<int>(params.SymAlgorithm);
                if (ImGui::Combo("Algorithm", &current_alg, alg_labels, IM_ARRAYSIZE(alg_labels))) {
                    params.SymAlgorithm = static_cast<SanmapGen::SymmetryAlgorithm>(current_alg);
                    bNeedsPreviewUpdate = true;
                }
                
                // Show context-sensitive sliders
                if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Blur) {
                    if (ImGui::SliderFloat("Blur Radius", &params.SymmetryBlurRadius, 1.0f, 50.0f)) bNeedsPreviewUpdate = true;
                } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::CrossFade) {
                    if (ImGui::SliderFloat("Blend Width (Radians)", &params.CrossFadeWidth, 0.01f, 1.0f)) bNeedsPreviewUpdate = true;
                } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Superposition) {
                    const char* blend_labels[] = { "Add", "Subtract", "Multiply", "Overlay", "Max", "Min" };
                    int current_blend = static_cast<int>(params.SymSuperpositionBlend);
                    if (ImGui::Combo("Superposition Blend", &current_blend, blend_labels, IM_ARRAYSIZE(blend_labels))) {
                        params.SymSuperpositionBlend = static_cast<SanmapGen::BlendMode>(current_blend);
                        bNeedsPreviewUpdate = true;
                    }
                } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Cylinder3D) {
                    if (ImGui::SliderFloat("Cylinder Stretch", &params.CylinderZScale, 0.1f, 10.0f)) bNeedsPreviewUpdate = true;
                } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Torus3D) {
                    if (ImGui::SliderFloat("Major Radius", &params.TorusMajorRadius, 10.0f, 500.0f)) bNeedsPreviewUpdate = true;
                    if (ImGui::SliderFloat("Minor Radius", &params.TorusMinorRadius, 10.0f, 500.0f)) bNeedsPreviewUpdate = true;
                }
                
                ImGui::Spacing();
                ImGui::Text("Global Hydraulic Erosion");
                ImGui::Separator();
                if (ImGui::Checkbox("Enable Stratified Erosion", &params.Erosion.Enabled)) bNeedsPreviewUpdate = true;
                if (params.Erosion.Enabled) {
                    ImGui::Checkbox("Use GPU Compute (GLSL)", &params.Erosion.UseGPU);
                    if (ImGui::DragInt("Droplet Count", &params.Erosion.DropletCount, 1000.0f, 1000, 1000000)) bNeedsPreviewUpdate = true;
                    if (ImGui::SliderInt("Max Lifetime", &params.Erosion.MaxLifetime, 10, 100)) bNeedsPreviewUpdate = true;
                    if (ImGui::SliderFloat("Evaporation Rate", &params.Erosion.EvaporationRate, 0.001f, 0.1f)) bNeedsPreviewUpdate = true;
                    if (ImGui::SliderFloat("Gravity", &params.Erosion.Gravity, 1.0f, 10.0f)) bNeedsPreviewUpdate = true;

                    ImGui::Spacing();
                    if (ImGui::TreeNodeEx("Precipitation (Rain Clouds)", ImGuiTreeNodeFlags_None)) {
                        if (ImGui::Checkbox("Enable Rain Noise", &params.Erosion.UseRainNoise)) bNeedsPreviewUpdate = true;
                        if (params.Erosion.UseRainNoise) {
                            if (ImGui::SliderFloat("Cloud Frequency", &params.Erosion.RainNoiseFreq, 0.001f, 0.1f)) bNeedsPreviewUpdate = true;
                            if (ImGui::SliderInt("Cloud Octaves", &params.Erosion.RainNoiseOctaves, 1, 8)) bNeedsPreviewUpdate = true;
                            if (ImGui::SliderFloat("Cloud Density (Threshold)", &params.Erosion.RainNoiseThreshold, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                        }
                        
                        ImGui::Spacing();
                        if (ImGui::Checkbox("Enable Orographic Rain (Rain Shadows)", &params.Erosion.UseOrographicRain)) bNeedsPreviewUpdate = true;
                        if (params.Erosion.UseOrographicRain) {
                            if (ImGui::SliderFloat("Wind Angle (Degrees)", &params.Erosion.WindAngle, 0.0f, 360.0f)) bNeedsPreviewUpdate = true;
                        }
                        ImGui::TreePop();
                    }
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Geological Stratum Layers (Bottom to Top)");
                ImGui::SameLine(ImGui::GetWindowWidth() - 100);
                if (ImGui::Button("Add Layer")) {
                    SanmapGen::NoiseLayer newLayer;
                    newLayer.Name = "New Layer " + std::to_string(params.Layers.size() + 1);
                    params.Layers.push_back(newLayer);
                    bNeedsPreviewUpdate = true;
                }
                ImGui::Separator();
                
                for (size_t i = 0; i < params.Layers.size(); ++i) {
                    auto& layer = params.Layers[i];
                    ImGui::PushID((int)i);
                    
                    if (ImGui::Checkbox("##enabled", &layer.Enabled)) bNeedsPreviewUpdate = true;
                    ImGui::SameLine();
                    
                    bool expanded = ImGui::CollapsingHeader(layer.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                    if (expanded) {
                        char nameBuf[128];
                        strncpy(nameBuf, layer.Name.c_str(), sizeof(nameBuf));
                        if (ImGui::InputText("Layer Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {
                            layer.Name = nameBuf;
                        }
                        
                        // Stratum Type
                        const char* stratum_labels[] = { "Bedrock", "Sand", "Silt", "Clay", "Loam", "Snow" };
                        if (ImGui::BeginCombo("Stratum Type", stratum_labels[(int)layer.Stratum])) {
                            for (int n = 0; n < IM_ARRAYSIZE(stratum_labels); n++) {
                                if (ImGui::Selectable(stratum_labels[n], ((int)layer.Stratum == n))) {
                                    layer.Stratum = (SanmapGen::StratumType)n;
                                    switch (layer.Stratum) {
                                        case SanmapGen::StratumType::Bedrock: layer.Hardness=1.0f; layer.Friction=0.1f; layer.Cohesion=1.0f; layer.CapacityMult=0.1f; break;
                                        case SanmapGen::StratumType::Sand:    layer.Hardness=0.2f; layer.Friction=0.8f; layer.Cohesion=0.5f; layer.CapacityMult=2.0f; break;
                                        case SanmapGen::StratumType::Silt:    layer.Hardness=0.4f; layer.Friction=0.7f; layer.Cohesion=0.7f; layer.CapacityMult=1.5f; break;
                                        case SanmapGen::StratumType::Clay:    layer.Hardness=0.6f; layer.Friction=0.9f; layer.Cohesion=0.9f; layer.CapacityMult=1.0f; break;
                                        case SanmapGen::StratumType::Loam:    layer.Hardness=0.3f; layer.Friction=0.8f; layer.Cohesion=0.6f; layer.CapacityMult=1.8f; break;
                                        case SanmapGen::StratumType::Snow:    layer.Hardness=0.05f; layer.Friction=0.2f; layer.Cohesion=0.4f; layer.CapacityMult=3.0f; break;
                                    }
                                    bNeedsPreviewUpdate = true;
                                }
                            }
                            ImGui::EndCombo();
                        }

                        if (ImGui::TreeNodeEx("Soil Physics Overrides", ImGuiTreeNodeFlags_None)) {
                            if (ImGui::SliderFloat("Hardness", &layer.Hardness, 0.01f, 1.0f)) bNeedsPreviewUpdate = true;
                            if (ImGui::SliderFloat("Friction", &layer.Friction, 0.01f, 1.0f)) bNeedsPreviewUpdate = true;
                            if (ImGui::SliderFloat("Cohesion (Talus Angle)", &layer.Cohesion, 0.01f, 1.0f)) bNeedsPreviewUpdate = true;
                            if (ImGui::SliderFloat("Capacity Multiplier", &layer.CapacityMult, 0.1f, 5.0f)) bNeedsPreviewUpdate = true;
                            ImGui::TreePop();
                        }
                        
                        if (ImGui::Checkbox("Erodable", &layer.Erodable)) bNeedsPreviewUpdate = true;

                        const char* blend_labels[] = { "Add", "Subtract", "Multiply", "Overlay", "Max", "Min" };
                        if (ImGui::BeginCombo("Blend Mode", blend_labels[(int)layer.Blend])) {
                            for (int n = 0; n < IM_ARRAYSIZE(blend_labels); n++) {
                                if (ImGui::Selectable(blend_labels[n], ((int)layer.Blend == n))) {
                                    layer.Blend = (SanmapGen::BlendMode)n;
                                    bNeedsPreviewUpdate = true;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        
                        const char* mode_labels[] = { "Procedural Noise", "Heightmap Image" };
                        int current_mode = layer.UseImage ? 1 : 0;
                        if (ImGui::BeginCombo("Mode", mode_labels[current_mode])) {
                            for (int n = 0; n < 2; n++) {
                                if (ImGui::Selectable(mode_labels[n], current_mode == n)) {
                                    layer.UseImage = (n == 1);
                                    bNeedsPreviewUpdate = true;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        
                        if (layer.UseImage) {
                            if (ImGui::Button("Select Image...")) {
                                std::string path;
                                if (SanmapGen::FileDialog::OpenFile("PNG Files\0*.png\0All Files\0*.*\0", path)) {
                                    layer.ImagePath = path;
                                    int channels;
                                    unsigned short* data = stbi_load_16(path.c_str(), &layer.ImageWidth, &layer.ImageHeight, &channels, 1);
                                    if (data) {
                                        layer.ImageData.resize(layer.ImageWidth * layer.ImageHeight);
                                        for (int p = 0; p < layer.ImageWidth * layer.ImageHeight; ++p) {
                                            layer.ImageData[p] = static_cast<float>(data[p]) / 65535.0f;
                                        }
                                        stbi_image_free(data);
                                        bNeedsPreviewUpdate = true;
                                    }
                                }
                            }
                            if (!layer.ImagePath.empty()) {
                                ImGui::TextWrapped("Loaded: %s", layer.ImagePath.c_str());
                                if (ImGui::SliderFloat("Opacity", &layer.Opacity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                            }
                        } else {
                            // Noise Properties UI
                            const char* noise_labels[] = { "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "ValueCubic", "Value" };
                        if (ImGui::BeginCombo("Noise Type", noise_labels[(int)layer.Type])) {
                            for (int n = 0; n < 6; n++) {
                                if (ImGui::Selectable(noise_labels[n], ((int)layer.Type == n))) {
                                    layer.Type = (SanmapGen::NoiseType)n;
                                    bNeedsPreviewUpdate = true;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        
                        const char* fractal_labels[] = { "None", "FBm", "Ridged", "PingPong" };
                        if (ImGui::BeginCombo("Fractal", fractal_labels[(int)layer.Fractal])) {
                            for (int n = 0; n < 4; n++) {
                                if (ImGui::Selectable(fractal_labels[n], ((int)layer.Fractal == n))) {
                                    layer.Fractal = (SanmapGen::FractalType)n;
                                    bNeedsPreviewUpdate = true;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        
                        ImGui::Text("Symmetry:"); ImGui::SameLine();
                        bool symPoint = (layer.SymmetryMask & SanmapGen::Symmetry_Point);
                        bool symX = (layer.SymmetryMask & SanmapGen::Symmetry_X);
                        bool symZ = (layer.SymmetryMask & SanmapGen::Symmetry_Z);
                        bool symXY = (layer.SymmetryMask & SanmapGen::Symmetry_XY);
                        bool symRadial = (layer.SymmetryMask & SanmapGen::Symmetry_Radial);
                        if (ImGui::Checkbox("Point", &symPoint)) { layer.SymmetryMask ^= SanmapGen::Symmetry_Point; bNeedsPreviewUpdate = true; } ImGui::SameLine();
                        if (ImGui::Checkbox("X", &symX)) { layer.SymmetryMask ^= SanmapGen::Symmetry_X; bNeedsPreviewUpdate = true; } ImGui::SameLine();
                        if (ImGui::Checkbox("Z", &symZ)) { layer.SymmetryMask ^= SanmapGen::Symmetry_Z; bNeedsPreviewUpdate = true; } ImGui::SameLine();
                        if (ImGui::Checkbox("XY", &symXY)) { layer.SymmetryMask ^= SanmapGen::Symmetry_XY; bNeedsPreviewUpdate = true; } ImGui::SameLine();
                        if (ImGui::Checkbox("Radial", &symRadial)) { layer.SymmetryMask ^= SanmapGen::Symmetry_Radial; bNeedsPreviewUpdate = true; }
                        
                        ImGui::Spacing(); ImGui::Text("Noise Properties");
                        if (ImGui::DragFloat("Frequency", &layer.Frequency, 0.001f, 0.0001f, 0.5f, "%.4f")) bNeedsPreviewUpdate = true;
                        if (ImGui::SliderInt("Octaves", &layer.Octaves, 1, 10)) bNeedsPreviewUpdate = true;
                        if (ImGui::SliderFloat("Gain", &layer.Gain, 0.1f, 5.0f)) bNeedsPreviewUpdate = true;
                        if (layer.Fractal == SanmapGen::FractalType::PingPong) {
                            if (ImGui::SliderFloat("PingPong", &layer.PingPongStrength, 0.1f, 5.0f)) bNeedsPreviewUpdate = true;
                        }
                        if (ImGui::SliderFloat("Opacity", &layer.Opacity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                        if (layer.Type == SanmapGen::NoiseType::Cellular) {
                            if (ImGui::SliderFloat("Jitter", &layer.CellularJitter, 0.0f, 2.0f)) bNeedsPreviewUpdate = true;
                        }
                        
                        ImGui::Spacing(); ImGui::Text("Density Shaping");
                        if (ImGui::SliderFloat("Land", &layer.LandDensity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                        if (ImGui::SliderFloat("Plateau", &layer.PlateauDensity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                        if (ImGui::SliderFloat("Mountain", &layer.MountainDensity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                        if (ImGui::SliderFloat("Ramp", &layer.RampDensity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                        } // End of else block for UseImage
                        
                        if (ImGui::Button("Duplicate Layer")) {
                            SanmapGen::NoiseLayer copiedLayer = layer;
                            copiedLayer.Name = copiedLayer.Name + " (Copy)";
                            params.Layers.insert(params.Layers.begin() + i + 1, copiedLayer);
                            bNeedsPreviewUpdate = true; ImGui::PopID(); break;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Delete Layer")) {
                            params.Layers.erase(params.Layers.begin() + i);
                            bNeedsPreviewUpdate = true; ImGui::PopID(); break;
                        }
                    }
                    ImGui::PopID();
                    ImGui::Separator();
                }

                ImGui::Spacing();
                if (ImGui::Button("Bake to Base Layer (Freeze)", ImVec2(-1, 30))) {
                    // Generate final map
                    SanmapGen::FloatMask tempMap(params.MapSize, params.MapSize, 0.0f);
                    SanmapGen::TerrainGenerator::GenerateMap(tempMap, params);
                    
                    // Save as 16-bit PNG
                    std::vector<unsigned short> out16(params.MapSize * params.MapSize);
                    for (int p = 0; p < params.MapSize * params.MapSize; ++p) {
                        out16[p] = static_cast<unsigned short>(tempMap.GetDataPtr()[p] * 65535.0f);
                    }
                    std::string outPath = "baked_base_layer.png";
                    stbi_write_png(outPath.c_str(), params.MapSize, params.MapSize, 1, out16.data(), params.MapSize * 2);
                    
                    // Disable all current layers
                    for (auto& l : params.Layers) l.Enabled = false;
                    
                    // Create new Base Layer
                    SanmapGen::NoiseLayer newLayer;
                    newLayer.Name = "Frozen Base Layer";
                    newLayer.Enabled = true;
                    newLayer.UseImage = true;
                    newLayer.ImagePath = outPath;
                    newLayer.ImageWidth = params.MapSize;
                    newLayer.ImageHeight = params.MapSize;
                    newLayer.ImageData.resize(params.MapSize * params.MapSize);
                    std::copy(tempMap.GetDataPtr(), tempMap.GetDataPtr() + (params.MapSize * params.MapSize), newLayer.ImageData.begin());
                    
                    newLayer.Erodable = false; // By default bedrock isn't erodable
                    newLayer.Stratum = SanmapGen::StratumType::Bedrock;
                    
                    // Add it as a new base layer
                    params.Layers.insert(params.Layers.begin(), newLayer);
                    
                    bNeedsPreviewUpdate = true;
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Stratums"))
            {
                ImGui::Checkbox("##showStratums", &params.ShowStratums); ImGui::SameLine();
                ImGui::Text("Material Strata Settings");
                ImGui::Separator();

                // 9 Stratums according to SanMap.cs
                for (int i = 0; i < params.Stratums.size(); ++i) {
                    ImGui::PushID(i);
                    char label[32];
                    snprintf(label, sizeof(label), "Stratum %d", i + 1);
                    if (ImGui::CollapsingHeader(label)) {
                        ImGui::ColorEdit4("Base Color", params.Stratums[i].BaseColor);
                        
                        char pathBuf[256];
                        strncpy(pathBuf, params.Stratums[i].AlbedoPath.c_str(), sizeof(pathBuf));
                        if (ImGui::InputText("Albedo Texture", pathBuf, IM_ARRAYSIZE(pathBuf))) params.Stratums[i].AlbedoPath = pathBuf;
                        
                        strncpy(pathBuf, params.Stratums[i].NormalPath.c_str(), sizeof(pathBuf));
                        if (ImGui::InputText("Normal Texture", pathBuf, IM_ARRAYSIZE(pathBuf))) params.Stratums[i].NormalPath = pathBuf;
                        
                        strncpy(pathBuf, params.Stratums[i].CompositePath.c_str(), sizeof(pathBuf));
                        if (ImGui::InputText("Composite Texture", pathBuf, IM_ARRAYSIZE(pathBuf))) params.Stratums[i].CompositePath = pathBuf;
                    }
                    ImGui::PopID();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Detail Normal"))
            {
                ImGui::Checkbox("##showDetailNormal", &params.ShowDetailNormal); ImGui::SameLine();
                ImGui::Text("Detail Normal Settings (Coming Soon)");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Tint"))
            {
                ImGui::Checkbox("##showTint", &params.ShowTint); ImGui::SameLine();
                ImGui::Text("Tint Settings (Coming Soon)");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Holes"))
            {
                ImGui::Checkbox("##showHoles", &params.ShowHoles); ImGui::SameLine();
                ImGui::Text("Holes Settings (Coming Soon)");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Smoothness"))
            {
                ImGui::Checkbox("##showSmoothness", &params.ShowSmoothness); ImGui::SameLine();
                ImGui::Text("Smoothness Settings (Coming Soon)");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Water"))
            {
                ImGui::Checkbox("##showWater", &params.ShowWater); ImGui::SameLine();
                ImGui::Text("Water & Waves");
                ImGui::Separator();
                
                if (ImGui::SliderFloat("Water Level Min", &params.Water.WaterLevelMin, 0.0f, 128.0f)) bNeedsPreviewUpdate = true;
                if (ImGui::SliderFloat("Water Level Max", &params.Water.WaterLevelMax, 0.0f, 128.0f)) bNeedsPreviewUpdate = true;
                if (ImGui::SliderFloat("Deep Water Min", &params.Water.DeepWaterDepthMin, 0.0f, 128.0f)) bNeedsPreviewUpdate = true;
                if (ImGui::SliderFloat("Deep Water Max", &params.Water.DeepWaterDepthMax, 0.0f, 128.0f)) bNeedsPreviewUpdate = true;
                
                char waveBuf[256];
                strncpy(waveBuf, params.Water.WaveGeneratorBlueprint.c_str(), sizeof(waveBuf));
                if (ImGui::InputText("Wave Blueprint", waveBuf, IM_ARRAYSIZE(waveBuf))) {
                    params.Water.WaveGeneratorBlueprint = waveBuf;
                }
                
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Markers"))
            {
                ImGui::Checkbox("##showMarkers", &params.ShowMarkers); ImGui::SameLine();
                ImGui::Text("Procedural Markers");
                ImGui::Separator();
                
                if (ImGui::Button("Add Marker Rule")) {
                    SanmapGen::MarkerRule newRule;
                    newRule.Name = "New Rule " + std::to_string(params.Markers.size() + 1);
                    params.Markers.push_back(newRule);
                    bNeedsPreviewUpdate = true;
                }
                
                if (ImGui::BeginTabBar("MarkerSubTabs")) {
                    for (size_t i = 0; i < params.Markers.size(); ++i) {
                        auto& rule = params.Markers[i];
                        ImGui::PushID((int)i);
                        if (ImGui::BeginTabItem(rule.Name.c_str())) {
                            
                            char nameBuf[128];
                            strncpy(nameBuf, rule.Name.c_str(), sizeof(nameBuf));
                            if (ImGui::InputText("Rule Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {
                                rule.Name = nameBuf;
                            }
                            
                            if (ImGui::Checkbox("Enabled", &rule.Enabled)) bNeedsPreviewUpdate = true;
                            
                            ImGui::Spacing();
                            ImGui::Text("Terrain Filters");
                            if (ImGui::DragFloatRange2("Slope Range (Degrees)", &rule.MinSlope, &rule.MaxSlope, 0.5f, 0.0f, 90.0f)) bNeedsPreviewUpdate = true;
                            if (ImGui::DragFloatRange2("Height Range", &rule.MinHeight, &rule.MaxHeight, 0.5f, 0.0f, 128.0f)) bNeedsPreviewUpdate = true;
                            
                            ImGui::Spacing();
                            ImGui::Text("Spawning");
                            if (ImGui::SliderFloat("Density", &rule.Density, 0.0f, 10.0f)) bNeedsPreviewUpdate = true;
                            
                            if (ImGui::Button("Delete Rule")) {
                                params.Markers.erase(params.Markers.begin() + i);
                                bNeedsPreviewUpdate = true;
                                ImGui::EndTabItem();
                                ImGui::PopID();
                                break;
                            }
                            
                            ImGui::EndTabItem();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTabBar();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Reclaim"))
            {
                ImGui::Checkbox("##showReclaim", &params.ShowReclaim); ImGui::SameLine();
                ImGui::Text("Reclaim Settings (Coming Soon)");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Props"))
            {
                ImGui::Checkbox("##showProps", &params.ShowProps); ImGui::SameLine();
                ImGui::Text("Props Settings (Coming Soon)");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Decals"))
            {
                ImGui::Checkbox("##showDecals", &params.ShowDecals); ImGui::SameLine();
                ImGui::Text("Decals Settings (Coming Soon)");
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Save / Export"))
            {
                ImGui::Text("Save Map Generator Settings (Presets)");
                ImGui::Separator();
                
                static std::string lastSavePath = "";
                
                if (ImGui::Button("Save Settings (preset.json)", ImVec2(-1, 30)))
                {
                    std::string path;
                    if (SanmapGen::FileDialog::SaveFile("JSON Files\0*.json\0All Files\0*.*\0", "json", path)) {
                        lastSavePath = path;
                        SanmapGen::MapExporter::SaveSettings(path, params);
                    }
                }
                
                if (ImGui::Button("Load Settings (preset.json)", ImVec2(-1, 30)))
                {
                    std::string path;
                    if (SanmapGen::FileDialog::OpenFile("JSON Files\0*.json\0All Files\0*.*\0", path)) {
                        lastSavePath = path;
                        SanmapGen::MapExporter::LoadSettings(path, params);
                        bNeedsPreviewUpdate = true;
                    }
                }
                
                if (!lastSavePath.empty()) {
                    ImGui::Text("Last Preset: %s", lastSavePath.c_str());
                }
                
                ImGui::Spacing();
                ImGui::Text("Export Game Map");
                ImGui::Separator();
                
                ImGui::Text("Gameplay Setup");
                if (ImGui::SliderInt("Spawn Points", &params.SpawnPointCount, 2, 16)) bNeedsPreviewUpdate = true;
                if (ImGui::SliderFloat("Alloy Multiplier", &params.AlloyMultiplier, 0.0f, 3.0f)) bNeedsPreviewUpdate = true;
                if (ImGui::SliderFloat("Hydro Multiplier", &params.HydroMultiplier, 0.0f, 3.0f)) bNeedsPreviewUpdate = true;
                
                ImGui::Spacing();
                
                static std::string lastExportPath = "Output";
                ImGui::InputText("Export Folder", (char*)lastExportPath.c_str(), 0, ImGuiInputTextFlags_ReadOnly);
                
                if (ImGui::Button("Select Export Folder", ImVec2(-1, 24))) {
                    std::string path;
                    if (SanmapGen::FileDialog::SelectFolder(path)) {
                        lastExportPath = path;
                    }
                }
                
                if (ImGui::Button("Export mapdef.sanmap & Textures", ImVec2(-1, 50)))
                {
                    SanmapGen::MapExporter::ExportSanmap(lastExportPath, params);
                }
                
                ImGui::Spacing();
                if (ImGui::Button("Export Final Heightmap (16-bit PNG)", ImVec2(-1, 30))) {
                    std::string path;
                    if (SanmapGen::FileDialog::SaveFile("PNG Files\0*.png\0", "png", path)) {
                        SanmapGen::FloatMask tempMap(params.MapSize, params.MapSize, 0.0f);
                        SanmapGen::TerrainGenerator::GenerateMap(tempMap, params);
                        std::vector<unsigned short> out16(params.MapSize * params.MapSize);
                        for (int p = 0; p < params.MapSize * params.MapSize; ++p) {
                            out16[p] = static_cast<unsigned short>(tempMap.GetDataPtr()[p] * 65535.0f);
                        }
                        stbi_write_png(path.c_str(), params.MapSize, params.MapSize, 1, out16.data(), params.MapSize * 2);
                    }
                }
                
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        
        ImGui::Spacing();
        // The button now serves as a manual force-refresh, but dragging sliders auto-updates!
        if (ImGui::Button("Force Generate Preview", ImVec2(-1, 40)) || bNeedsPreviewUpdate)
        {
            bNeedsPreviewUpdate = false; // Reset the flag
            SanmapGen::FloatMask dummyMap(params.MapSize, params.MapSize, 0.0f);
            
            // Generate real procedural terrain using the threaded TGUE Morton Generator!
            SanmapGen::TerrainGenerator::GenerateMap(dummyMap, params);
            
            // Upload the procedural map to the GPU
            previewTexture = SanmapGen::PreviewRenderer::UpdatePreviewTexture(dummyMap, params, previewTexture);
        }
        
        ImGui::End(); // End Settings Window

        // --- WINDOW 2: PREVIEW ---
        ImGui::SetNextWindowSize(ImVec2(550, 550), ImGuiCond_FirstUseEver);
        ImGui::Begin("Map Preview");
        
        if (previewTexture == 0) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Click 'Generate Preview' to render.");
        } else {
            // Get available window size to draw the image as large as possible
            ImVec2 availSize = ImGui::GetContentRegionAvail();
            
            // Keep the image square
            float renderSize = availSize.x < availSize.y ? availSize.x : availSize.y;
            
            // Render the OpenGL texture id
            ImGui::Image((void*)(intptr_t)previewTexture, ImVec2(renderSize, renderSize));
        }

        ImGui::End(); // End Preview Window

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Update and Render additional Platform Windows (Viewports)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

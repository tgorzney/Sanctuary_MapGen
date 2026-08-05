#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <string>
#include <math.h>
#include <algorithm>
#include <filesystem>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include "Parameters.h"
#include "PreviewRenderer.h"
#include "TerrainGenerator.h"
#include "MapExporter.h"
#include "FileDialog.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "miniz.h"


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Scans the .sanpack for a given material and sets the Albedo/Normal/Composite paths
void ScanSanpackForMaterial(const std::string& zipPath, const std::string& environmentTheme, const std::string& materialName, SanmapGen::StratumSettings& stratum) {
    if (zipPath.empty() || materialName.empty()) return;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) {
        return; // failed to open
    }
    
    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    std::string prefix = environmentTheme + "/Stratum/";
    
    for (int i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        
        std::string fname = file_stat.m_filename;
        // Check if file is inside the correct environment and contains the material name
        if (fname.find(environmentTheme) != std::string::npos && fname.find("Stratum") != std::string::npos && fname.find(materialName) != std::string::npos) {
            if (fname.find("_albedo.dds") != std::string::npos || fname.find("_albedo.png") != std::string::npos) {
                stratum.AlbedoPath = fname;
            } else if (fname.find("_normal.dds") != std::string::npos || fname.find("_normal.png") != std::string::npos) {
                stratum.NormalPath = fname;
            } else if (fname.find("_mask.dds") != std::string::npos || fname.find("_masks.dds") != std::string::npos || fname.find("_mask.png") != std::string::npos) {
                stratum.CompositePath = fname;
            }
        }
    }
    mz_zip_reader_end(&zip_archive);
}

// Scans the .sanpack for environments (subfolders in the root)
std::vector<std::string> GetEnvironmentsFromSanpack(const std::string& zipPath) {
    std::vector<std::string> envs;
    if (zipPath.empty()) return envs;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) return envs;
    
    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        std::string fname = file_stat.m_filename;
        size_t firstSlash = fname.find('/');
        if (firstSlash != std::string::npos && firstSlash > 0) {
            std::string rootFolder = fname.substr(0, firstSlash);
            if (std::find(envs.begin(), envs.end(), rootFolder) == envs.end()) {
                envs.push_back(rootFolder);
            }
        }
    }
    mz_zip_reader_end(&zip_archive);
    return envs;
}

// Scans the .sanpack for materials within an environment
std::vector<std::string> GetMaterialsFromSanpack(const std::string& zipPath, const std::string& env) {
    std::vector<std::string> mats;
    if (zipPath.empty() || env.empty()) return mats;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) return mats;
    
    std::string prefix = env + "/Stratum/";
    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        std::string fname = file_stat.m_filename;
        if (fname.find(env) != std::string::npos && fname.find("Stratum") != std::string::npos && !mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
            // parse out the material name (e.g. highlands_100m_grass01_albedo.dds -> highlands_100m_grass01)
            size_t lastSlash = fname.find_last_of('/');
            std::string basename = (lastSlash != std::string::npos) ? fname.substr(lastSlash + 1) : fname;
            
            size_t underscore = basename.rfind("_albedo");
            if (underscore == std::string::npos) underscore = basename.rfind("_normal");
            if (underscore == std::string::npos) underscore = basename.rfind("_mask");
            if (underscore != std::string::npos) {
                std::string mat = basename.substr(0, underscore);
                if (std::find(mats.begin(), mats.end(), mat) == mats.end()) {
                    mats.push_back(mat);
                }
            }
        }
    }
    mz_zip_reader_end(&zip_archive);
    return mats;
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

                // --- Save / Load Preset ---
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.40f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.20f, 1.0f));
                if (ImGui::Button("Save Preset", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2.0f, 26))) {
                    std::string path;
                    if (SanmapGen::FileDialog::SaveFile("JSON Preset\0*.json\0", "json", path)) {
                        SanmapGen::MapExporter::SaveSettings(path, params);
                    }
                }
                ImGui::PopStyleColor(2);
                ImGui::SameLine(0, 4);
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.25f, 0.45f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.35f, 0.60f, 1.0f));
                if (ImGui::Button("Load Preset", ImVec2(-1, 26))) {
                    std::string path;
                    if (SanmapGen::FileDialog::OpenFile("JSON Preset\0*.json\0All Files\0*.*\0", path)) {
                        SanmapGen::MapExporter::LoadSettings(path, params);
                        bNeedsPreviewUpdate = true;
                    }
                }
                ImGui::PopStyleColor(2);
                ImGui::Spacing();
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
                ImGui::Text("Hydraulic Erosion");
                ImGui::Separator();
                ImGui::TextDisabled("Erosion is now configured per-layer.");
                ImGui::TextDisabled("Expand a layer below and open");
                ImGui::TextDisabled("'Hydraulic Erosion' or 'Deposition (Soil Drop)'.");
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Geological Stratum Layers (Bottom to Top)");
                ImGui::Separator();
                
                for (int s = 8; s >= 0; --s) {
                    ImGui::PushID(s * 1000);
                    
                    std::string headerName = params.Stratums[s].Name;
                    if (s == 0) headerName += " (Bottom)";
                    else if (s == 8) headerName += " (Top)";
                    
                    bool stratExpanded = ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                    
                    // Drag and Drop Target for Stratum
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_DRAG")) {
                            IM_ASSERT(payload->DataSize == sizeof(size_t));
                            size_t source_i = *(const size_t*)payload->Data;
                            if (source_i < params.Layers.size()) {
                                if (ImGui::GetIO().KeyCtrl) {
                                    SanmapGen::NoiseLayer copiedLayer = params.Layers[source_i];
                                    copiedLayer.Name = copiedLayer.Name + " (Copy)";
                                    copiedLayer.StratumIndex = s;
                                    params.Layers.insert(params.Layers.begin() + source_i + 1, copiedLayer);
                                } else {
                                    params.Layers[source_i].StratumIndex = s;
                                }
                                bNeedsPreviewUpdate = true;
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    
                    if (stratExpanded) {
                        ImGui::Indent();
                        
                        if (ImGui::Button("Add Layer to Stratum")) {
                            SanmapGen::NoiseLayer newLayer;
                            newLayer.Name = "New Layer " + std::to_string(params.Layers.size() + 1);
                            newLayer.StratumIndex = s;
                            params.Layers.push_back(newLayer);
                            bNeedsPreviewUpdate = true;
                        }
                        
                        for (size_t i = 0; i < params.Layers.size(); ++i) {
                            if (params.Layers[i].StratumIndex != s) continue;
                            
                            auto& layer = params.Layers[i];
                            ImGui::PushID((int)i);
                            
                            ImGui::Spacing();

                            // ---- LAYER HEADER: Dark gray full-width bar ----
                            float headerWidth = ImGui::GetContentRegionAvail().x;
                            ImVec2 headerPos = ImGui::GetCursorScreenPos();
                            float headerHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;

                            // Draw dark gray background
                            ImGui::GetWindowDrawList()->AddRectFilled(
                                headerPos,
                                ImVec2(headerPos.x + headerWidth, headerPos.y + headerHeight),
                                IM_COL32(45, 45, 48, 255)
                            );

                            // Enabled checkbox
                            if (ImGui::Checkbox("##enabled", &layer.Enabled)) bNeedsPreviewUpdate = true;
                            ImGui::SameLine();

                            // Layer Name (collapsible, full-width drag source)
                            ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderActive,   ImVec4(0.30f, 0.30f, 0.33f, 1.0f));
                            bool expanded = ImGui::CollapsingHeader(layer.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth);
                            ImGui::PopStyleColor(3);

                            // Full-width Drag Source on the header
                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                size_t payload_i = i;
                                ImGui::SetDragDropPayload("LAYER_DRAG", &payload_i, sizeof(size_t));
                                ImGui::Text("Moving: %s", layer.Name.c_str());
                                ImGui::EndDragDropSource();
                            }

                            // --- Right-aligned header buttons ---
                            const float btnWidth = 80.0f;
                            const float btnGap   = 4.0f;
                            float buttonsWidth = btnWidth * 3.0f + btnGap * 2.0f;
                            ImGui::SameLine(headerWidth - buttonsWidth);

                            // Bake Button
                            if (ImGui::Button("Bake", ImVec2(btnWidth, 0))) {
                                // Generate the terrain up to and including this layer
                                SanmapGen::FloatMask tempMap(params.MapSize, params.MapSize, 0.0f);
                                SanmapGen::TerrainGenerator::GenerateMap(tempMap, params);

                                // Create output folder
                                std::string bakedDir = "Output/BakedLayers";
                                std::filesystem::create_directories(bakedDir);

                                // Safe filename
                                std::string safeName = layer.Name;
                                for (char& c : safeName) if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') c = '_';
                                std::string pngPath  = bakedDir + "/" + safeName + "_" + std::to_string(i) + ".png";
                                std::string jsonPath = bakedDir + "/" + safeName + "_" + std::to_string(i) + ".json";

                                // Save 16-bit PNG of the composite
                                std::vector<unsigned short> out16(params.MapSize * params.MapSize);
                                for (int p = 0; p < params.MapSize * params.MapSize; ++p)
                                    out16[p] = static_cast<unsigned short>(tempMap.GetDataPtr()[p] * 65535.0f);
                                stbi_write_png(pngPath.c_str(), params.MapSize, params.MapSize, 1, out16.data(), params.MapSize * 2);

                                // Save JSON sidecar with all layer settings
                                SanmapGen::MapExporter::SaveSettings(jsonPath, params); // reuse full preset for now; Unbake re-loads it

                                // Switch layer to image mode
                                layer.OriginPresetPath = jsonPath;
                                layer.ImagePath = pngPath;
                                layer.ImageWidth = params.MapSize;
                                layer.ImageHeight = params.MapSize;
                                layer.ImageData.resize(params.MapSize * params.MapSize);
                                std::copy(tempMap.GetDataPtr(), tempMap.GetDataPtr() + params.MapSize * params.MapSize, layer.ImageData.begin());
                                layer.UseImage = true;

                                bNeedsPreviewUpdate = true;
                            }
                            ImGui::SameLine(0, btnGap);

                            // Duplicate Button
                            if (ImGui::Button("Duplicate", ImVec2(btnWidth, 0))) {
                                SanmapGen::NoiseLayer copiedLayer = layer;
                                copiedLayer.Name = copiedLayer.Name + " (Copy)";
                                params.Layers.insert(params.Layers.begin() + i + 1, copiedLayer);
                                bNeedsPreviewUpdate = true; ImGui::PopID(); break;
                            }
                            ImGui::SameLine(0, btnGap);

                            // Delete Button
                            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
                            if (ImGui::Button("Delete", ImVec2(btnWidth, 0))) {
                                params.Layers.erase(params.Layers.begin() + i);
                                bNeedsPreviewUpdate = true; ImGui::PopStyleColor(2); ImGui::PopID(); break;
                            }
                            ImGui::PopStyleColor(2);
                            // ------------------------------------------------

                            if (expanded) {
                                ImGui::Indent();

                                char nameBuf[128];
                                strncpy(nameBuf, layer.Name.c_str(), sizeof(nameBuf));
                                if (ImGui::InputText("Layer Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {
                                    layer.Name = nameBuf;
                                }

                                // --- Stratum assignment ---
                                ImGui::Text("Stratum:"); ImGui::SameLine();
                                ImGui::SetNextItemWidth(80);
                                if (ImGui::InputInt("##stratumIdx", &layer.StratumIndex)) {
                                    layer.StratumIndex = std::clamp(layer.StratumIndex, 0, 8);
                                    bNeedsPreviewUpdate = true;
                                }

                                // --- Per-Layer Soil Physics ---
                                if (ImGui::TreeNodeEx("Soil Physics", ImGuiTreeNodeFlags_None)) {
                                    if (ImGui::SliderFloat("Hardness",            &layer.Hardness,      0.01f, 1.0f))  bNeedsPreviewUpdate = true;
                                    if (ImGui::SliderFloat("Friction",            &layer.Friction,      0.01f, 1.0f))  bNeedsPreviewUpdate = true;
                                    if (ImGui::SliderFloat("Cohesion (Talus)",    &layer.Cohesion,      0.01f, 1.0f))  bNeedsPreviewUpdate = true;
                                    if (ImGui::SliderFloat("Capacity Multiplier", &layer.CapacityMult,  0.1f,  5.0f))  bNeedsPreviewUpdate = true;
                                    ImGui::TreePop();
                                }

                                // --- Per-Layer Hydraulic Erosion ---
                                if (ImGui::TreeNodeEx("Hydraulic Erosion", ImGuiTreeNodeFlags_None)) {
                                    if (ImGui::Checkbox("Enable Erosion##ero", &layer.Erosion.Enabled)) bNeedsPreviewUpdate = true;
                                    if (layer.Erosion.Enabled) {
                                        
                                        if (ImGui::DragInt("Droplet Count##edc", &layer.Erosion.DropletCount, 1000.0f, 1000, 5000000)) bNeedsPreviewUpdate = true;
                                        if (ImGui::SliderInt("Max Lifetime##elt", &layer.Erosion.MaxLifetime, 5, 200))   bNeedsPreviewUpdate = true;
                                        if (ImGui::SliderFloat("Evaporation##eev", &layer.Erosion.EvaporationRate, 0.001f, 0.2f)) bNeedsPreviewUpdate = true;
                                        if (ImGui::SliderFloat("Gravity##egr",     &layer.Erosion.Gravity, 0.5f, 20.0f))           bNeedsPreviewUpdate = true;

                                        if (ImGui::Checkbox("Erode Layers Beneath##eb", &layer.ErodeBeneath)) bNeedsPreviewUpdate = true;
                                        if (layer.ErodeBeneath) {
                                            ImGui::TextDisabled("  Droplets can dig into layers below this one");
                                        }

                                        ImGui::Spacing();
                                        if (ImGui::TreeNodeEx("Precipitation##prec", ImGuiTreeNodeFlags_None)) {
                                            if (ImGui::Checkbox("Rain Noise##rn", &layer.Erosion.UseRainNoise)) bNeedsPreviewUpdate = true;
                                            if (layer.Erosion.UseRainNoise) {
                                                if (ImGui::SliderFloat("Cloud Frequency##rnf",   &layer.Erosion.RainNoiseFreq,      0.001f, 0.1f)) bNeedsPreviewUpdate = true;
                                                if (ImGui::SliderInt("Cloud Octaves##rno",        &layer.Erosion.RainNoiseOctaves,   1, 8))         bNeedsPreviewUpdate = true;
                                                if (ImGui::SliderFloat("Cloud Density##rnt",     &layer.Erosion.RainNoiseThreshold, 0.0f, 1.0f))   bNeedsPreviewUpdate = true;
                                            }
                                            ImGui::Spacing();
                                            if (ImGui::Checkbox("Orographic Rain##or", &layer.Erosion.UseOrographicRain)) bNeedsPreviewUpdate = true;
                                            if (layer.Erosion.UseOrographicRain) {
                                                if (ImGui::SliderFloat("Wind Angle##wa", &layer.Erosion.WindAngle, 0.0f, 360.0f)) bNeedsPreviewUpdate = true;
                                            }
                                            ImGui::TreePop();
                                        }
                                    }
                                    ImGui::TreePop();
                                }

                                // --- Deposition (Soil Drop) Pass ---
                                if (ImGui::TreeNodeEx("Deposition (Soil Drop)", ImGuiTreeNodeFlags_None)) {
                                    ImGui::TextDisabled("Spawn droplets pre-loaded with sediment to simulate");
                                    ImGui::TextDisabled("material settling (loam, clay, dirt, sand).");
                                    ImGui::Spacing();
                                    if (ImGui::Checkbox("Enable Deposition Mode##dm", &layer.Erosion.DepositionMode)) bNeedsPreviewUpdate = true;
                                    if (layer.Erosion.DepositionMode) {
                                        if (!layer.Erosion.Enabled) {
                                            layer.Erosion.Enabled = true; // deposition requires erosion pass
                                        }
                                        if (ImGui::SliderFloat("Initial Sediment Load##isl", &layer.Erosion.InitialSedimentLoad, 0.01f, 5.0f)) bNeedsPreviewUpdate = true;
                                        ImGui::Spacing();
                                        ImGui::Text("Spawn Height Range (0=bottom, 1=top)");
                                        if (ImGui::SliderFloat("Min Height##smn", &layer.Erosion.SpawnMinHeight, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                                        if (ImGui::SliderFloat("Max Height##smx", &layer.Erosion.SpawnMaxHeight, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                                        if (layer.Erosion.SpawnMinHeight > layer.Erosion.SpawnMaxHeight)
                                            layer.Erosion.SpawnMinHeight = layer.Erosion.SpawnMaxHeight;
                                    }
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

                                // --- Mode: Procedural Noise or Image ---
                                // Unbake button if this layer was baked
                                if (layer.UseImage && !layer.OriginPresetPath.empty()) {
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.45f, 0.7f, 1.0f));
                                    if (ImGui::Button("Unbake (Restore Procedural)", ImVec2(-1, 0))) {
                                        // Restore to procedural: just clear UseImage
                                        layer.UseImage    = false;
                                        layer.ImagePath   = "";
                                        layer.ImageData.clear();
                                        layer.ImageWidth  = 0;
                                        layer.ImageHeight = 0;
                                        // OriginPresetPath kept so user can re-bake
                                        bNeedsPreviewUpdate = true;
                                    }
                                    ImGui::PopStyleColor();
                                    ImGui::TextDisabled("Baked from: %s", layer.OriginPresetPath.c_str());
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
                                                for (int p = 0; p < layer.ImageWidth * layer.ImageHeight; ++p)
                                                    layer.ImageData[p] = static_cast<float>(data[p]) / 65535.0f;
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
                                    bool symPoint  = (layer.SymmetryMask & SanmapGen::Symmetry_Point);
                                    bool symX      = (layer.SymmetryMask & SanmapGen::Symmetry_X);
                                    bool symZ      = (layer.SymmetryMask & SanmapGen::Symmetry_Z);
                                    bool symXY     = (layer.SymmetryMask & SanmapGen::Symmetry_XY);
                                    bool symRadial = (layer.SymmetryMask & SanmapGen::Symmetry_Radial);
                                    if (ImGui::Checkbox("Point",  &symPoint))  { layer.SymmetryMask ^= SanmapGen::Symmetry_Point;  bNeedsPreviewUpdate = true; } ImGui::SameLine();
                                    if (ImGui::Checkbox("X",      &symX))      { layer.SymmetryMask ^= SanmapGen::Symmetry_X;      bNeedsPreviewUpdate = true; } ImGui::SameLine();
                                    if (ImGui::Checkbox("Z",      &symZ))      { layer.SymmetryMask ^= SanmapGen::Symmetry_Z;      bNeedsPreviewUpdate = true; } ImGui::SameLine();
                                    if (ImGui::Checkbox("XY",     &symXY))     { layer.SymmetryMask ^= SanmapGen::Symmetry_XY;     bNeedsPreviewUpdate = true; } ImGui::SameLine();
                                    if (ImGui::Checkbox("Radial", &symRadial)) { layer.SymmetryMask ^= SanmapGen::Symmetry_Radial; bNeedsPreviewUpdate = true; }

                                    ImGui::Spacing(); ImGui::Text("Noise Properties");
                                    if (ImGui::DragFloat("Frequency", &layer.Frequency, 0.001f, 0.0001f, 0.5f, "%.4f")) bNeedsPreviewUpdate = true;
                                    if (ImGui::SliderInt("Octaves",   &layer.Octaves, 1, 10))           bNeedsPreviewUpdate = true;
                                    if (ImGui::SliderFloat("Gain",    &layer.Gain, 0.1f, 5.0f))         bNeedsPreviewUpdate = true;
                                    if (layer.Fractal == SanmapGen::FractalType::PingPong) {
                                        if (ImGui::SliderFloat("PingPong", &layer.PingPongStrength, 0.1f, 5.0f)) bNeedsPreviewUpdate = true;
                                    }
                                    if (ImGui::SliderFloat("Opacity", &layer.Opacity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                                    if (layer.Type == SanmapGen::NoiseType::Cellular) {
                                        if (ImGui::SliderFloat("Jitter", &layer.CellularJitter, 0.0f, 2.0f)) bNeedsPreviewUpdate = true;
                                    }

                                    ImGui::Spacing(); ImGui::Text("Density Shaping");
                                    if (ImGui::SliderFloat("Land",     &layer.LandDensity,     0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                                    if (ImGui::SliderFloat("Plateau",  &layer.PlateauDensity,  0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                                    if (ImGui::SliderFloat("Mountain", &layer.MountainDensity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                                    if (ImGui::SliderFloat("Ramp",     &layer.RampDensity,     0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                                }

                                ImGui::Unindent();
                            }
                            ImGui::PopID();
                        }
                        ImGui::Unindent();
                    }
                    ImGui::PopID();
                }

                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Stratums"))
            {
                ImGui::Checkbox("##showStratums", &params.ShowStratums); ImGui::SameLine();
                ImGui::Text("Material Strata Settings");
                ImGui::Separator();
                
                if (ImGui::Button("Environment")) {
                    std::string path;
                    if (SanmapGen::FileDialog::OpenFile("Sanctuary Packs\0*.sanpack;*.zip\0All Files\0*.*\0", path)) {
                        params.GlobalEnvironmentPath = path;
                    }
                }
                if (!params.GlobalEnvironmentPath.empty()) {
                    ImGui::TextWrapped("Env: %s", params.GlobalEnvironmentPath.c_str());
                }
                
                ImGui::Separator();

                // 9 Stratums according to SanMap.cs
                for (int i = 0; i < params.Stratums.size(); ++i) {
                    ImGui::PushID(i);
                    char label[64];
                    snprintf(label, sizeof(label), "Stratum %d - %s", i, params.Stratums[i].Name.c_str());
                    if (ImGui::CollapsingHeader(label)) {
                        char nameBuf[128];
                        strncpy(nameBuf, params.Stratums[i].Name.c_str(), sizeof(nameBuf));
                        if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) params.Stratums[i].Name = nameBuf;
                        
                        ImGui::Spacing();
                        ImGui::Text("Environment Textures & Theme");
                        if (!params.GlobalEnvironmentPath.empty()) {
                            // Theme Dropdown
                            std::vector<std::string> envs = GetEnvironmentsFromSanpack(params.GlobalEnvironmentPath);
                            if (envs.size() > 0) {
                                if (params.Stratums[i].EnvironmentTheme.empty()) params.Stratums[i].EnvironmentTheme = envs[0];
                                
                                if (ImGui::BeginCombo("Theme Folder", params.Stratums[i].EnvironmentTheme.c_str())) {
                                    for (auto& e : envs) {
                                        if (ImGui::Selectable(e.c_str(), params.Stratums[i].EnvironmentTheme == e)) {
                                            params.Stratums[i].EnvironmentTheme = e;
                                        }
                                    }
                                    ImGui::EndCombo();
                                }
                                
                                // Material Dropdown
                                std::vector<std::string> mats = GetMaterialsFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].EnvironmentTheme);
                                if (mats.size() > 0 && params.Stratums[i].MaterialName.empty()) {
                                    params.Stratums[i].MaterialName = mats[0];
                                    ScanSanpackForMaterial(params.GlobalEnvironmentPath, params.Stratums[i].EnvironmentTheme, mats[0], params.Stratums[i]);
                                }
                                if (ImGui::BeginCombo("Material Name", params.Stratums[i].MaterialName.c_str())) {
                                    for (auto& m : mats) {
                                        if (ImGui::Selectable(m.c_str(), params.Stratums[i].MaterialName == m)) {
                                            params.Stratums[i].MaterialName = m;
                                            ScanSanpackForMaterial(params.GlobalEnvironmentPath, params.Stratums[i].EnvironmentTheme, m, params.Stratums[i]);
                                        }
                                    }
                                    ImGui::EndCombo();
                                }
                            } else {
                                ImGui::TextDisabled("No themes found in sanpack.");
                            }
                        } else {
                            ImGui::TextDisabled("Select an environment .sanpack at the top.");
                        }

                        ImGui::Spacing();
                        ImGui::ColorEdit4("Base Color", params.Stratums[i].BaseColor);
                        ImGui::ColorEdit4("Tint Color", params.Stratums[i].Tint);
                        ImGui::ColorEdit4("Color Override", params.Stratums[i].ColorOverride);
                        ImGui::SliderFloat("Tint Blend", &params.Stratums[i].TintBlend, 0.0f, 1.0f);
                        
                        char pathBuf[256];
                        strncpy(pathBuf, params.Stratums[i].AlbedoPath.c_str(), sizeof(pathBuf));
                        if (ImGui::InputText("Albedo Texture", pathBuf, IM_ARRAYSIZE(pathBuf))) params.Stratums[i].AlbedoPath = pathBuf;
                        
                        strncpy(pathBuf, params.Stratums[i].NormalPath.c_str(), sizeof(pathBuf));
                        if (ImGui::InputText("Normal Texture", pathBuf, IM_ARRAYSIZE(pathBuf))) params.Stratums[i].NormalPath = pathBuf;
                        
                        strncpy(pathBuf, params.Stratums[i].CompositePath.c_str(), sizeof(pathBuf));
                        if (ImGui::InputText("Composite Texture", pathBuf, IM_ARRAYSIZE(pathBuf))) params.Stratums[i].CompositePath = pathBuf;
                        
                        ImGui::Spacing();
                        ImGui::Text("Mask Remapping (Composite Channels)");
                        ImGui::SliderFloat4("Mask Max", params.Stratums[i].MaskRemapMax, 0.0f, 1.0f);
                        ImGui::SliderFloat4("Mask Min", params.Stratums[i].MaskRemapMin, 0.0f, 1.0f);
                        
                        ImGui::Spacing();
                        ImGui::Text("Tiling & Scaling");
                        ImGui::DragFloat2("Near Tiling", params.Stratums[i].NearTiling, 0.1f);
                        ImGui::DragFloat2("Far Tiling", params.Stratums[i].FarTiling, 0.1f);
                        ImGui::SliderFloat("Near Normal Scale", &params.Stratums[i].NearNormalScale, 0.0f, 5.0f);
                        ImGui::SliderFloat("Far Normal Scale", &params.Stratums[i].FarNormalScale, 0.0f, 5.0f);
                        
                        ImGui::Spacing();
                        ImGui::Text("Blending");
                        if (i > 0) { // Stratum 0 does not use masks/blends
                            ImGui::Checkbox("Fill Darker Areas First (Dirt Logic)", &params.Stratums[i].UseDarkerAreaFill);
                            ImGui::SliderFloat("Height Blend Depth", &params.Stratums[i].HeightBlendDepth, 0.0f, 10.0f);
                            ImGui::SliderFloat("Height Blend Contrast", &params.Stratums[i].HeightBlendContrast, 0.0f, 5.0f);
                        }
                        ImGui::SliderFloat("Normal Near Blend", &params.Stratums[i].NormalNearBlend, 0.0f, 1.0f);
                        ImGui::SliderFloat("Height Near Blend", &params.Stratums[i].HeightNearBlend, 0.0f, 1.0f);
                        ImGui::SliderFloat("Fade Begin", &params.Stratums[i].FadeBegin, 0.0f, 1000.0f);
                        ImGui::SliderFloat("Fade Distance", &params.Stratums[i].FadeDistance, 0.0f, 1000.0f);
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
            
            // Sort layers by StratumIndex to ensure correct geological order (0 to 8) during evaluation
            std::stable_sort(params.Layers.begin(), params.Layers.end(), [](const SanmapGen::NoiseLayer& a, const SanmapGen::NoiseLayer& b) {
                return a.StratumIndex < b.StratumIndex;
            });
            
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

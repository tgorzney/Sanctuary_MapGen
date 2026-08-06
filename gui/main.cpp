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
#include "UITabs.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Scans the .sanpack for a given material and sets the Albedo/Normal/Composite paths
void ScanSanpackForMaterial(const std::string& zipPath, const std::string& environmentTheme, const std::string& materialName, SanmapGen::StratumSettings& stratum) {
    if (zipPath.empty() || materialName.empty()) return;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) return;
    
    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    std::string prefix = environmentTheme + "/Stratum/";
    
    for (int i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        
        std::string fname = file_stat.m_filename;
        if (fname.find(environmentTheme) != std::string::npos && fname.find("Stratum") != std::string::npos && fname.find(materialName) != std::string::npos) {
            if (fname.find("_albedo.dds") != std::string::npos || fname.find("_albedo.png") != std::string::npos) {
                stratum.AlbedoPath = fname;
            } else if (fname.find("_normal.dds") != std::string::npos || fname.find("_normal.png") != std::string::npos) {
                stratum.NormalPath = fname;
            } else if (fname.find("_mask.dds") != std::string::npos || fname.find("_masks.dds") != std::string::npos || fname.find("_mask.png") != std::string::npos) {
                stratum.MaskPath = fname;
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
        
        size_t stratumPos = fname.find("/Stratum/");
        if (stratumPos != std::string::npos) {
            std::string envFolder = fname.substr(0, stratumPos);
            if (std::find(envs.begin(), envs.end(), envFolder) == envs.end()) {
                envs.push_back(envFolder);
            }
        } else if (fname.find("Stratum/") == 0) {
            std::string envFolder = "Root";
            if (std::find(envs.begin(), envs.end(), envFolder) == envs.end()) {
                envs.push_back(envFolder);
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
    
    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        std::string fname = file_stat.m_filename;
        if (fname.find(env) != std::string::npos && fname.find("Stratum") != std::string::npos && !mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
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

// RenderLayerHeader and inline UI logic has been moved to gui/TerrainTabs.cpp and gui/UITabs.h

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 430";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Sanctuary Map Generator", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    SanmapGen::GenerationParams params;
    GLuint previewTexture = 0;
    bool bNeedsMapUpdate = true;
    bool bNeedsPreviewRender = true;
    bool bGeometryChanged = true;
    int initVertSize = params.MapSize + 1;
    SanmapGen::FloatMask dummyMap(initVertSize, initVertSize, 0.0f);
    std::vector<SanmapGen::FloatMask> stratums;
    SanmapGen::GenerationResult genResult;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static float leftPaneWidth = 180.0f;
        
        // --- GENERATOR SETTINGS WINDOW ---
        ImGui::SetNextWindowSize(ImVec2(650, 720), ImGuiCond_FirstUseEver);
        ImGui::Begin("Generator Settings");
        
        // LEFT PANE - TABS
        ImGui::BeginChild("LeftPane", ImVec2(leftPaneWidth, 0), true);
        static int activeTab = 0;
        
        auto TabButton = [&](const char* label, bool& showVar, int tabIndex) {
            ImGui::PushID(tabIndex);
            if (ImGui::Button(showVar ? "[O]" : "[ ]")) { showVar = !showVar; bNeedsMapUpdate = true; }
            ImGui::SameLine();
            if (ImGui::Selectable(label, activeTab == tabIndex)) { activeTab = tabIndex; }
            ImGui::PopID();
        };

        auto TabButtonNoToggle = [&](const char* label, int tabIndex) {
            ImGui::PushID(tabIndex);
            if (ImGui::Selectable(label, activeTab == tabIndex)) { activeTab = tabIndex; }
            ImGui::PopID();
        };

        ImGui::Text("TERRAIN & LAYERS");
        ImGui::Separator();
        TabButton("Heightmap", params.ShowHeightmap, 0);
        TabButton("Slope Map", params.ShowSlopeMap, 1);
        TabButton("Flow Map", params.ShowFlowMap, 13);
        TabButton("Accumulation", params.ShowAccumulationMap, 14);
        TabButton("Stratums", params.ShowStratums, 2);
        TabButton("Detail Normal", params.ShowDetailNormal, 3);
        TabButton("Tint", params.ShowTint, 4);
        TabButton("Holes", params.ShowHoles, 5);
        TabButton("Smoothness", params.ShowSmoothness, 6);
        
        ImGui::Spacing();
        ImGui::Text("ENVIRONMENT");
        ImGui::Separator();
        TabButton("Water", params.ShowWater, 7);
        TabButton("Atmosphere", params.ShowAtmosphere, 8);
        TabButton("Markers", params.ShowMarkers, 9);
        TabButton("Props", params.ShowProps, 10);
        
        ImGui::Spacing();
        ImGui::Text("SYSTEM");
        ImGui::Separator();
        if (ImGui::Selectable("Performance", activeTab == 11)) activeTab = 11;
        if (ImGui::Selectable("Files / Save", activeTab == 12)) activeTab = 12;
        
        ImGui::Spacing(); ImGui::Spacing();
        
        bool isInteracting = ImGui::IsAnyItemActive();
        params.FastPreviewMode = isInteracting;
        
        static bool wasInteracting = false;
        if (wasInteracting && !isInteracting) {
            bNeedsMapUpdate = true; // Trigger final full update to calculate Flow/Placements when drag finishes
        }
        wasInteracting = isInteracting;
        
        if (ImGui::Button("Force Generate", ImVec2(-1, 40)) || bNeedsMapUpdate) {
            bNeedsMapUpdate = false;
            bGeometryChanged = true;
            
            int vertSize = params.MapSize + 1;
            if (dummyMap.GetWidth() != vertSize || dummyMap.GetHeight() != vertSize) {
                dummyMap = SanmapGen::FloatMask(vertSize, vertSize, 0.0f);
            }
            
            SanmapGen::TerrainGenerator::GenerateMap(dummyMap, params, genResult);
            stratums = genResult.Stratums;
            bNeedsPreviewRender = true;
        }
        
        if (bNeedsPreviewRender) {
            bNeedsPreviewRender = false;
            previewTexture = SanmapGen::PreviewRenderer::UpdatePreviewTexture(dummyMap, genResult, params, previewTexture, bGeometryChanged);
            bGeometryChanged = false;
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        ImGui::Button("##Splitter1", ImVec2(5, -1));
        if (ImGui::IsItemActive()) { leftPaneWidth += ImGui::GetIO().MouseDelta.x; if (leftPaneWidth < 100) leftPaneWidth = 100; }
        ImGui::SameLine();
        
        // RIGHT PANE - SETTINGS
        ImGui::BeginChild("SettingsPane", ImVec2(0, 0), true);
        switch (activeTab) {
            case 0: SanmapGen::UI::RenderHeightmapTab(params, bNeedsMapUpdate); break;
            case 1: SanmapGen::UI::RenderSlopeMapTab(params, bNeedsPreviewRender); break;
            case 13: SanmapGen::UI::RenderFlowMapTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 14: SanmapGen::UI::RenderAccumulationMapTab(params, bNeedsPreviewRender); break;
            case 2: SanmapGen::UI::RenderStratumsTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 3: SanmapGen::UI::RenderDetailNormalTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 4: SanmapGen::UI::RenderTintTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 5: SanmapGen::UI::RenderHolesTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 6: SanmapGen::UI::RenderSmoothnessTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 7: SanmapGen::UI::RenderWaterTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 8: SanmapGen::UI::RenderAtmosphereTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 9: SanmapGen::UI::RenderMarkersTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 10: SanmapGen::UI::RenderPropsTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 11: SanmapGen::UI::RenderPerformanceTab(params, bNeedsMapUpdate); break;
            case 12: SanmapGen::UI::RenderSaveExportTab(params, dummyMap, genResult, bNeedsMapUpdate); break;
        }
        ImGui::EndChild(); // SettingsPane
        
        ImGui::End(); // Generator Settings Window
        
        
        // --- MAP PREVIEW WINDOW ---
        ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Map Preview");
        static bool showCompositeSettings = false;
        
        if (ImGui::Button("[ View ]")) {
            ImGui::OpenPopup("ViewPopup");
        }
        ImGui::SameLine();
        if (ImGui::Button("[ Order ]")) showCompositeSettings = true;
        
        ImGui::SameLine();
        if (ImGui::Checkbox("Auto-Level", &params.AutoLevelPreview)) {
            bNeedsPreviewRender = true;
        }
        
        const char* blendModeNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Divide", "Overlay", "Screen", "Soft Light", "Hard Light" };
        
        if (ImGui::BeginPopup("ViewPopup")) {
            ImGui::Text("Map Layers");
            ImGui::Separator();
            for (int i = (int)params.PreviewLayers.size() - 1; i >= 0; --i) {
                auto& layer = params.PreviewLayers[i];
                ImGui::PushID(i);
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%-15s", layer.Name.c_str());
                ImGui::SameLine(120);
                
                ImGui::SetNextItemWidth(100);
                int current_blend = (int)layer.Blend;
                if (ImGui::Combo("##blend", &current_blend, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
                    layer.Blend = (SanmapGen::GenerationParams::LayerBlendMode)current_blend;
                    
                    bool enabled = (layer.Blend != SanmapGen::GenerationParams::LayerBlendMode::None);
                    switch (layer.Type) {
                        case SanmapGen::GenerationParams::PreviewLayerType::Heightmap: params.ShowHeightmap = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Slope: params.ShowSlopeMap = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Flow: params.ShowFlowMap = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Accumulation: params.ShowAccumulationMap = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Stratums: params.ShowStratums = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Water: params.ShowWater = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Markers: params.ShowMarkers = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Props: params.ShowProps = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::DetailNormal: params.ShowDetailNormal = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Tint: params.ShowTint = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Holes: params.ShowHoles = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Smoothness: params.ShowSmoothness = enabled; break;
                    }
                    bNeedsPreviewRender = true;
                }
                
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }
        ImGui::Separator();
        
        if (showCompositeSettings) {
            ImGui::Begin("Composite Layer Settings", &showCompositeSettings);
            ImGui::Text("Composite Layers (Drag to Reorder, Top-to-Bottom)");
            for (int i = (int)params.PreviewLayers.size() - 1; i >= 0; --i) {
                ImGui::PushID(i);
                ImGui::Selectable(params.PreviewLayers[i].Name.c_str(), false, 0, ImVec2(0, 20));
                
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("PREVIEW_LAYER_DRAG", &i, sizeof(int));
                    ImGui::Text("Moving %s", params.PreviewLayers[i].Name.c_str());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREVIEW_LAYER_DRAG")) {
                        int source_i = *(const int*)payload->Data;
                        if (source_i != i) {
                            auto movingLayer = params.PreviewLayers[source_i];
                            params.PreviewLayers.erase(params.PreviewLayers.begin() + source_i);
                            int insert_i = (source_i < i) ? i - 1 : i;
                            params.PreviewLayers.insert(params.PreviewLayers.begin() + insert_i, movingLayer);
                            bNeedsPreviewRender = true;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::PopID();
            }
            ImGui::End();
        }
        
        if (previewTexture == 0) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Preview texture not generated yet.");
        } else {
            ImVec2 availSize = ImGui::GetContentRegionAvail();
            float renderSize = availSize.x < availSize.y ? availSize.x : availSize.y;
            
            static ImVec2 mapOffset(0.0f, 0.0f);
            static float mapZoom = 1.0f;
            
            // Interaction logic
            ImGui::InvisibleButton("MapCanvas", ImVec2(renderSize, renderSize));
            bool isHovered = ImGui::IsItemHovered();
            bool isActive = ImGui::IsItemActive();
            ImVec2 p0 = ImGui::GetItemRectMin();
            ImVec2 p1 = ImGui::GetItemRectMax();
            
            if (isHovered && ImGui::GetIO().MouseWheel != 0.0f) {
                // Zoom
                float mouseWheel = ImGui::GetIO().MouseWheel;
                float zoomFactor = powf(1.1f, mouseWheel);
                
                // Keep the mouse position fixed in world space while zooming
                ImVec2 mousePos = ImGui::GetIO().MousePos;
                ImVec2 uvMouse = ImVec2((mousePos.x - p0.x) / renderSize, (mousePos.y - p0.y) / renderSize);
                
                ImVec2 oldCenterOffset = ImVec2(uvMouse.x - 0.5f, uvMouse.y - 0.5f);
                
                mapZoom *= zoomFactor;
                if (mapZoom < 1.0f) { mapZoom = 1.0f; mapOffset = ImVec2(0,0); }
                if (mapZoom > 50.0f) mapZoom = 50.0f;
                
                // Adjust offset to keep mouse point still
                mapOffset.x += oldCenterOffset.x * (1.0f / (mapZoom / zoomFactor) - 1.0f / mapZoom);
                mapOffset.y += oldCenterOffset.y * (1.0f / (mapZoom / zoomFactor) - 1.0f / mapZoom);
            }
            
            if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                mapOffset.x -= delta.x / renderSize / mapZoom;
                mapOffset.y -= delta.y / renderSize / mapZoom;
            }
            
            // Constrain offset
            float maxOffset = 0.5f - 0.5f / mapZoom;
            mapOffset.x = std::clamp(mapOffset.x, -maxOffset, maxOffset);
            mapOffset.y = std::clamp(mapOffset.y, -maxOffset, maxOffset);
            
            ImVec2 uv0 = ImVec2(0.5f - 0.5f / mapZoom + mapOffset.x, 0.5f - 0.5f / mapZoom + mapOffset.y);
            ImVec2 uv1 = ImVec2(0.5f + 0.5f / mapZoom + mapOffset.x, 0.5f + 0.5f / mapZoom + mapOffset.y);
            
            ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)previewTexture, p0, p1, uv0, uv1);
        }
        ImGui::End(); // Map Preview Window

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

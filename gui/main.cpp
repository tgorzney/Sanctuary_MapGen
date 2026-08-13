#include <unordered_set>
#include <future>
#include <chrono>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <string>
#include <math.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include "Parameters.h"
#include "PreviewRenderer.h"
#include "widgets/Widget_MapCanvas.h"
#include "TerrainGenerator.h"
#include "export/Export_Metadata.h"
#include "FileDialog.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "miniz.h"
#include "UITabs.h"
#include "../core/TextureLoader.h"
#include "../core/UnitParser.h"

using json = nlohmann::json;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static bool bHasLoadedIcons = false;

// Global helper to load and cache marker icons dynamically
GLuint GetMarkerIcon(const std::string& typeName, SanmapGen::GenerationParams& params, void* openZipArchive = nullptr) {
    if (typeName.empty()) return 0;
    auto it = params.IconCache.find(typeName);
    if (it != params.IconCache.end()) return it->second;

    std::string typeLower = typeName;
    std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);
    std::string exactNameIconLower = "UI/Sprites/Icons/Resources/" + typeLower + "_icon";
    std::string exactNameLower = "UI/Sprites/Icons/Resources/" + typeLower;
    
    std::string uiPack = params.GamedataPath + "/UI.sanpack";
    std::string localDebug = "";
    
    GLuint t = 0;
    
    // First try loose folder (only search inside UI/Sprites/Icons, searching all Gamedata takes multiple seconds on Windows)
    std::string looseIconsDir = params.GamedataPath + "/UI/Sprites/Icons";
    if (std::filesystem::exists(looseIconsDir) && std::filesystem::is_directory(looseIconsDir)) {
        try {
            std::error_code ec;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(looseIconsDir, std::filesystem::directory_options::skip_permission_denied, ec)) {
                if (ec) continue;
                if (!entry.is_regular_file(ec)) continue;
                std::string filename = entry.path().filename().string();
                std::string lower = filename;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                std::string search1 = typeLower + "_icon";
                std::string search2 = typeLower;
                std::string search3 = "icon_" + typeLower;
                
                if (lower.find(search1) != std::string::npos || lower.find(search3) != std::string::npos || (lower.find(search2) != std::string::npos && lower.find(".dds") != std::string::npos)) {
                    auto ends_with = [](const std::string& str, const std::string& suffix) {
                        return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
                    };
                    if (ends_with(lower, ".png") || ends_with(lower, ".jpg") || ends_with(lower, ".tga")) {
                        t = SanmapGen::TextureLoader::LoadImageFromFile(entry.path().string());
                    } else if (ends_with(lower, ".dds")) {
                        t = SanmapGen::TextureLoader::LoadDDSFromFile(entry.path().string(), &localDebug);
                    }
                    if (t != 0) break;
                }
            }
        } catch (...) {}
    }
    
    // Then try sanpack
    if (t == 0 && std::filesystem::exists(uiPack)) {
        if (std::filesystem::is_directory(uiPack)) {
            std::string iconsPath = uiPack + "/UI/Sprites/Icons";
            if (std::filesystem::exists(iconsPath)) {
                try {
                    std::error_code ec;
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(iconsPath, std::filesystem::directory_options::skip_permission_denied, ec)) {
                        if (ec) continue;
                        if (!entry.is_regular_file(ec)) continue;
                        std::string basename = entry.path().filename().string();
                        std::string lower = basename;
                        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                        
                        std::string lowerNoExt = lower.substr(0, lower.find_last_of('.'));
                        if (lowerNoExt == "icon_" + typeLower || lowerNoExt == typeLower + "_icon") {
                            auto ends_with = [](const std::string& str, const std::string& suffix) {
                                return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
                            };
                            if (ends_with(lower, ".png") || ends_with(lower, ".jpg")) {
                                t = SanmapGen::TextureLoader::LoadImageFromFile(entry.path().string());
                            } else if (ends_with(lower, ".dds")) {
                                t = SanmapGen::TextureLoader::LoadDDSFromFile(entry.path().string(), &localDebug);
                            }
                            if (t != 0) break;
                        }
                    }
                } catch (...) {}
            }
        } else {
            // It's a zip archive
            static bool s_ZipScanned = false;
            static std::unordered_map<std::string, std::string> s_IconToZipPath;
            static std::unordered_map<std::string, int> s_IconToZipIndex;
            
            if (!s_ZipScanned) {
                mz_zip_archive local_zip = {};
                mz_zip_archive* local_zip_ptr = nullptr;
                bool needsClose = false;
                
                if (openZipArchive) {
                    local_zip_ptr = static_cast<mz_zip_archive*>(openZipArchive);
                } else {
                    if (mz_zip_reader_init_file(&local_zip, uiPack.c_str(), 0)) {
                        local_zip_ptr = &local_zip;
                        needsClose = true;
                    }
                }
                
                if (local_zip_ptr) {
                    for (int i = 0; i < (int)mz_zip_reader_get_num_files(local_zip_ptr); ++i) {
                        mz_zip_archive_file_stat file_stat;
                        if (!mz_zip_reader_file_stat(local_zip_ptr, i, &file_stat)) continue;
                        
                        std::string originalName = file_stat.m_filename;
                        std::string name = originalName;
                        for (char& c : name) { if (c == '\\') c = '/'; }
                        std::string lowerName = name;
                        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                        
                        if (lowerName.find("ui/sprites/icons/") != std::string::npos) {
                            std::string basename = name.substr(name.find_last_of('/') + 1);
                            std::string lowerBase = basename;
                            std::transform(lowerBase.begin(), lowerBase.end(), lowerBase.begin(), ::tolower);
                            if (lowerBase.find("icon_") != std::string::npos || lowerBase.find("_icon") != std::string::npos || lowerBase.find("_symbol") != std::string::npos) {
                                auto ends_with = [](const std::string& str, const std::string& suffix) {
                                    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
                                };
                                if (ends_with(lowerBase, ".dds") || ends_with(lowerBase, ".png") || ends_with(lowerBase, ".jpg")) {
                                    std::string typeName = basename;
                                    size_t dotPos = typeName.find_last_of('.');
                                    if (dotPos != std::string::npos) typeName = typeName.substr(0, dotPos);
                                    
                                    std::string lowerType = typeName;
                                    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
                                    size_t idx = lowerType.find("icon");
                                    if (idx != std::string::npos) { typeName.erase(idx, 4); lowerType.erase(idx, 4); }
                                    idx = lowerType.find("_");
                                    if (idx != std::string::npos) { typeName.erase(idx, 1); lowerType.erase(idx, 1); }
                                    
                                    lowerType = typeName;
                                    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
                                    
                                    s_IconToZipPath[lowerType] = originalName;
                                    s_IconToZipIndex[lowerType] = i;
                                }
                            }
                        }
                    }
                    if (needsClose) mz_zip_reader_end(local_zip_ptr);
                }
                s_ZipScanned = true;
            }
            
            std::string targetPath = "";
            int targetIndex = -1;
            
            if (s_IconToZipPath.find(typeLower) != s_IconToZipPath.end()) {
                targetPath = s_IconToZipPath[typeLower];
                targetIndex = s_IconToZipIndex[typeLower];
            }
            
            if (!targetPath.empty()) {
                auto ends_with = [](const std::string& str, const std::string& suffix) {
                    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
                };
                std::string lowerPath = targetPath;
                std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
                
                if (ends_with(lowerPath, ".dds")) {
                    t = SanmapGen::TextureLoader::LoadDDSFromArchive(uiPack, targetPath, &localDebug, openZipArchive, targetIndex);
                } else if (ends_with(lowerPath, ".png") || ends_with(lowerPath, ".jpg")) {
                    t = SanmapGen::TextureLoader::LoadImageFromArchive(uiPack, targetPath, &localDebug, openZipArchive, targetIndex);
                }
            } else {
                localDebug += "Could not resolve '" + typeName + "' in zip cache.\n";
            }
        }
    }
    if (t == 0 && !localDebug.empty()) {
        params.DebugInfo += localDebug + "\n";
    }
    
    params.IconCache[typeName] = t; // Cache it even if it's 0 so we don't spam load attempts
    return t;
}

void ForceScanIcons(SanmapGen::GenerationParams& params) {
    if (params.GamedataPath.empty()) return;
    std::string uiPack = params.GamedataPath + "/UI.sanpack";
    params.IconScanDebugInfo = ""; // Clear old debug info
    if (std::filesystem::exists(uiPack)) {
        std::vector<std::string> scanned = SanmapGen::TextureLoader::ScanSanpackForMarkers(uiPack, &params.IconScanDebugInfo);
        if (!scanned.empty()) {
            params.KnownMarkerTypes = scanned;
            std::string cacheFile = params.GamedataPath + "/icons_cache.json";
            try {
                json j; j["KnownMarkerTypes"] = params.KnownMarkerTypes;
                std::ofstream o(cacheFile); o << j.dump(4);
                params.IconScanDebugInfo += "Successfully saved " + std::to_string(scanned.size()) + " markers to icons_cache.json\n";
            } catch(...) {
                params.IconScanDebugInfo += "Failed to save icons_cache.json\n";
            }
        } else {
            params.IconScanDebugInfo += "Warning: Scan returned empty list. Using defaults.\n";
        }
    } else {
        params.IconScanDebugInfo += "UI.sanpack does not exist at " + uiPack + "\n";
    }
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
    bool bResetPreviewTransform = false; // Set to true after LoadSanmap to snap preview camera to default
    int initVertSize = params.MapSize + 1;
    SanmapGen::FloatMask dummyMap(initVertSize, initVertSize, 0.0f);
    std::vector<SanmapGen::FloatMask> stratums;
    SanmapGen::GenerationResult genResult;

    
    std::future<std::pair<SanmapGen::FloatMask, SanmapGen::GenerationResult>> genFuture;
    bool bIsGenerating = false;
    SanmapGen::GenerationResult pendingGenResult;
    SanmapGen::FloatMask pendingDummyMap(initVertSize, initVertSize, 0.0f);
    SanmapGen::GenerationParams pendingParams;

    while (!glfwWindowShouldClose(window))
    {
        if (bNeedsMapUpdate || bNeedsPreviewRender || bIsGenerating || ImGui::GetIO().WantSetMousePos) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static std::string lastGamedataPath = "";
        if (lastGamedataPath != params.GamedataPath) {
            bHasLoadedIcons = false;
            lastGamedataPath = params.GamedataPath;
        }

        static bool bTriggerCacheOptimization = false;
        
        if (!bHasLoadedIcons && !params.GamedataPath.empty()) {
            if (!bTriggerCacheOptimization) {
                ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                bool open = ImGui::Begin("OptimizingCache", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav);
                if (open) {
                    ImGui::Text("Optimizing Cache... Please Wait (This may take a few seconds)");
                }
                ImGui::End();
                bTriggerCacheOptimization = true;
                // Force a render so the user sees the popup BEFORE the blocking parse happens on the next frame
                ImGui::Render();
                int display_w, display_h;
                glfwGetFramebufferSize(window, &display_w, &display_h);
                glViewport(0, 0, display_w, display_h);
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glfwSwapBuffers(window);
                continue;
            }
            
            bTriggerCacheOptimization = false;
            params.DebugInfo = "";
            
            // Initialize Global Archive for synchronous loading
            std::string uiPack = params.GamedataPath + "/UI.sanpack";
            std::string cacheFile = params.GamedataPath + "/icons_cache.json";
            
            mz_zip_archive global_ui_archive = {};
            mz_zip_archive* global_ui_archive_ptr = nullptr;
            if (std::filesystem::exists(uiPack)) {
                if (mz_zip_reader_init_file(&global_ui_archive, uiPack.c_str(), 0)) {
                    global_ui_archive_ptr = &global_ui_archive;
                }
            }
            
            // Load Unit Templates directly from Lua tables (or fast Cache)
            SanmapGen::UnitParser::LoadUnitDefinitions(params, global_ui_archive_ptr);
            
            if (std::filesystem::exists(cacheFile)) {
                try {
                    std::ifstream i(cacheFile);
                    json j; i >> j;
                    if (j.contains("AvailableIcons")) {
                        params.AvailableIcons = j["AvailableIcons"].get<std::vector<std::string>>();
                    }
                } catch (...) {}
            }
            
            if (params.AvailableIcons.empty()) {
                if (std::filesystem::exists(uiPack)) {
                    std::vector<std::string> scanned = SanmapGen::TextureLoader::ScanSanpackForMarkers(uiPack, &params.DebugInfo, global_ui_archive_ptr);
                    if (!scanned.empty()) {
                        params.AvailableIcons = scanned;
                        try {
                            json j; j["AvailableIcons"] = params.AvailableIcons;
                            std::ofstream o(cacheFile); o << j.dump(4);
                        } catch(...) {}
                    }
                }
            }
            // Clear old cache when gamedata path changes
            for (auto& kv : params.IconCache) {
                if (kv.second) glDeleteTextures(1, &kv.second);
            }
            params.IconCache.clear();
            
            // PRELOAD ALL ICONS SO THE SELECTOR IS INSTANT
            if (global_ui_archive_ptr && !params.AvailableIcons.empty()) {
                for (const auto& tName : params.AvailableIcons) {
                    GetMarkerIcon(tName, params, global_ui_archive_ptr);
                }
            }
            
            if (global_ui_archive_ptr) {
                mz_zip_reader_end(global_ui_archive_ptr);
            }
            
            SanmapGen::AsyncTextureManager::Init(params.GamedataPath);
            bHasLoadedIcons = true;
        }

        static float leftPaneWidth = 180.0f;
        
        // --- GENERATOR SETTINGS WINDOW ---
        ImGui::SetNextWindowSize(ImVec2(650, 720), ImGuiCond_FirstUseEver);
        ImGui::Begin("Generator Settings");
        
        // LEFT PANE - TABS
        ImGui::BeginChild("LeftPane", ImVec2(leftPaneWidth, 0), true);
        static int activeTab = 0;
        static std::string selectedMarkerKey = "";
        
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
        TabButton("Symmetry", params.ShowSymmetry, 15);
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
        TabButton("Armies", params.ShowArmies, 16);
        TabButton("Props", params.ShowProps, 10);
        TabButton("Areas", params.ShowAreas, 17);
        
        ImGui::Spacing();
        ImGui::Text("SYSTEM");
        ImGui::Separator();
        if (ImGui::Selectable("Performance", activeTab == 11)) activeTab = 11;
        if (ImGui::Selectable("Files / Save", activeTab == 12)) activeTab = 12;
        
        ImGui::Spacing(); ImGui::Spacing();
        
        bool isInteracting = ImGui::IsAnyItemActive();
        params.FastPreviewMode = isInteracting;
        
        static size_t lastHash = params.GetHash();
        size_t currentHash = params.GetHash();
        
        static bool wasInteracting = false;
        static bool changedDuringInteraction = false;
        
        if (isInteracting) {
            if (currentHash != lastHash || bNeedsMapUpdate) changedDuringInteraction = true;
        } else if (wasInteracting) {
            if (changedDuringInteraction) {
                bNeedsMapUpdate = true;
                changedDuringInteraction = false;
            }
        } else {
            if (currentHash != lastHash) {
                bNeedsMapUpdate = true;
            }
        }
        
        wasInteracting = isInteracting;
        lastHash = currentHash;
        
        if (ImGui::Button("Force Generate", ImVec2(-1, 40))) {
            bNeedsMapUpdate = true;
        }
        
        if (bNeedsMapUpdate && !bIsGenerating) {
            bNeedsMapUpdate = false;
            bIsGenerating = true;
            
            int vertSize = params.MapSize + 1;
            if (dummyMap.GetWidth() != vertSize || dummyMap.GetHeight() != vertSize) {
                dummyMap = SanmapGen::FloatMask(vertSize, vertSize, 0.0f);
            }
            
            pendingDummyMap = dummyMap;
            pendingGenResult = genResult;
            pendingParams = params;
            
            // Capture by value for safety, except for pointers which are managed safely
            genFuture = std::async(std::launch::async, [pendingDummyMap, pendingParams, pendingGenResult]() mutable {
                SanmapGen::TerrainGenerator::GenerateMap(pendingDummyMap, pendingParams, pendingGenResult);
                return std::make_pair(pendingDummyMap, pendingGenResult);
            });
        }
        
        if (bIsGenerating) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1,0.5f,0,1), "Generating Map...");
            
            if (genFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                auto result = genFuture.get(); // finalize
                bIsGenerating = false;
                
                // Commit changes
                dummyMap = result.first;
                genResult = result.second;
                stratums = genResult.Stratums;
                
                
                
                // Merge procedural markers
                for (auto it = params.MarkersList.begin(); it != params.MarkersList.end(); ) {
                    if (!it->second.IsManual) {
                        it = params.MarkersList.erase(it);
                    } else {
                        ++it;
                    }
                }
                for (const auto& kvp : genResult.GeneratedMarkers) {
                    params.MarkersList[kvp.first] = kvp.second;
                }
                
                bNeedsPreviewRender = true;
                bGeometryChanged = true;
            }
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
            case 15: SanmapGen::UI::RenderSymmetryTab(params, bNeedsMapUpdate); break;
            case 0: SanmapGen::UI::RenderHeightmapTab(params, bNeedsMapUpdate); break;
            case 1: SanmapGen::UI::RenderSlopeMapTab(params, bNeedsPreviewRender); break;
            case 13: SanmapGen::UI::RenderFlowMapTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 14: SanmapGen::UI::RenderAccumulationMapTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 2: SanmapGen::UI::RenderStratumsTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 3: SanmapGen::UI::RenderDetailNormalTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 4: SanmapGen::UI::RenderTintTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 5: SanmapGen::UI::RenderHolesTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 6: SanmapGen::UI::RenderSmoothnessTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 7: SanmapGen::UI::RenderWaterTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 8: SanmapGen::UI::RenderAtmosphereTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 9: SanmapGen::UI::RenderMarkersTab(params, selectedMarkerKey, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 16: SanmapGen::UI::RenderArmiesTab(params, bNeedsMapUpdate); break;
            case 10: SanmapGen::UI::RenderPropsTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 17: SanmapGen::UI::RenderAreasTab(params, bNeedsPreviewRender); break;
            case 11: SanmapGen::UI::RenderPerformanceTab(params, bNeedsMapUpdate); break;
            case 12: SanmapGen::UI::RenderSaveExportTab(params, dummyMap, genResult, bNeedsMapUpdate, bResetPreviewTransform); break;
        }
        ImGui::EndChild(); // SettingsPane
        
        ImGui::End(); // Generator Settings Window
        
        
        // --- MAP PREVIEW WINDOW ---
        SanmapGen::Widget_MapCanvas::Render(params, previewTexture, bNeedsMapUpdate, activeTab, selectedMarkerKey, bNeedsPreviewRender, bResetPreviewTransform, dummyMap);
        
        SanmapGen::AsyncTextureManager::ProcessReadyQueue(params);

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

    SanmapGen::AsyncTextureManager::Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

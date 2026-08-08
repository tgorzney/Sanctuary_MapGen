#include <unordered_set>
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
#include "TerrainGenerator.h"
#include "MapExporter.h"
#include "FileDialog.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "miniz.h"
#include "UITabs.h"
#include "../core/TextureLoader.h"

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
        for (const auto& entry : std::filesystem::recursive_directory_iterator(looseIconsDir)) {
            if (!entry.is_regular_file()) continue;
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
    }
    
    // Then try sanpack
    if (t == 0 && std::filesystem::exists(uiPack)) {
        if (std::filesystem::is_directory(uiPack)) {
            std::string iconsPath = uiPack + "/UI/Sprites/Icons";
            if (std::filesystem::exists(iconsPath)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(iconsPath)) {
                    if (!entry.is_regular_file()) continue;
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
    bool bResetPreviewTransform = false; // Set to true after LoadSanmap to snap preview camera to default
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

        static std::string lastGamedataPath = "";
        if (lastGamedataPath != params.GamedataPath) {
            bHasLoadedIcons = false;
            lastGamedataPath = params.GamedataPath;
        }

        if (!bHasLoadedIcons && !params.GamedataPath.empty()) {
            params.DebugInfo = "";
            std::string uiPack = params.GamedataPath + "/UI.sanpack";
            std::string cacheFile = params.GamedataPath + "/icons_cache.json";
            
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
                    std::vector<std::string> scanned = SanmapGen::TextureLoader::ScanSanpackForMarkers(uiPack);
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
            if (std::filesystem::exists(uiPack) && !params.AvailableIcons.empty()) {
                mz_zip_archive zip_archive = {};
                if (mz_zip_reader_init_file(&zip_archive, uiPack.c_str(), 0)) {
                    for (const auto& tName : params.AvailableIcons) {
                        GetMarkerIcon(tName, params, &zip_archive);
                    }
                    mz_zip_reader_end(&zip_archive);
                }
            }
            
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
        TabButton("Props", params.ShowProps, 10);
        
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
        
        if (ImGui::Button("Force Generate", ImVec2(-1, 40)) || bNeedsMapUpdate) {
            bNeedsMapUpdate = false;
            bGeometryChanged = true;
            
            int vertSize = params.MapSize + 1;
            if (dummyMap.GetWidth() != vertSize || dummyMap.GetHeight() != vertSize) {
                dummyMap = SanmapGen::FloatMask(vertSize, vertSize, 0.0f);
            }
            
            SanmapGen::TerrainGenerator::GenerateMap(dummyMap, params, genResult);
            stratums = genResult.Stratums;
            params.TerrainMinHeight = genResult.TerrainMinHeight;
            params.TerrainMaxHeight = genResult.TerrainMaxHeight;
            
            // Merge procedural markers (delete old procedural ones first)
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
            case 14: SanmapGen::UI::RenderAccumulationMapTab(params, bNeedsPreviewRender); break;
            case 2: SanmapGen::UI::RenderStratumsTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 3: SanmapGen::UI::RenderDetailNormalTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 4: SanmapGen::UI::RenderTintTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 5: SanmapGen::UI::RenderHolesTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 6: SanmapGen::UI::RenderSmoothnessTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 7: SanmapGen::UI::RenderWaterTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 8: SanmapGen::UI::RenderAtmosphereTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 9: SanmapGen::UI::RenderMarkersTab(params, selectedMarkerKey, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 10: SanmapGen::UI::RenderPropsTab(params, bNeedsMapUpdate, bNeedsPreviewRender); break;
            case 11: SanmapGen::UI::RenderPerformanceTab(params, bNeedsMapUpdate); break;
            case 12: SanmapGen::UI::RenderSaveExportTab(params, dummyMap, genResult, bNeedsMapUpdate, bResetPreviewTransform); break;
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
            
            // Reset preview transform when a new sanmap is loaded
            if (bResetPreviewTransform) {
                mapZoom = 1.0f;
                mapOffset = ImVec2(0.0f, 0.0f);
                bResetPreviewTransform = false;
            }
            
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
            
                        static std::string draggingMarker = "";
            static bool isDraggingMarker = false;
            static ImVec2 dragOffset(0, 0);
            
            if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !isDraggingMarker) {
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


            ImVec2 mousePos = ImGui::GetIO().MousePos;
            bool isHoveringPreview = isHovered;

            if (params.ShowMarkers && dummyMap.GetWidth() == params.MapSize + 1) {
                int spawnIndex = 0;
                ImU32 spawnColors[] = {
                    IM_COL32(255, 0, 0, 255),    // Red
                    IM_COL32(255, 255, 0, 255),  // Yellow
                    IM_COL32(255, 128, 0, 255),  // Orange
                    IM_COL32(0, 0, 255, 255),    // Blue
                    IM_COL32(0, 255, 0, 255),    // Green
                    IM_COL32(128, 0, 128, 255),  // Purple
                    IM_COL32(255, 192, 203, 255),// Pink
                    IM_COL32(255, 0, 255, 255),  // Magenta
                    IM_COL32(0, 128, 128, 255)   // Teal
                };
                int numSpawnColors = sizeof(spawnColors) / sizeof(spawnColors[0]);
                
                // Determine screen position for each marker
                std::unordered_set<std::string> hiddenMarkers;
                for (const auto& layer : params.PlacedMarkerLayers) {
                    if (!layer.Enabled) {
                        for (const auto& k : layer.MarkerKeys) hiddenMarkers.insert(k);
                    }
                }
                
                for (auto& [key, marker] : params.MarkersList) {
                    if (marker.IsHidden || hiddenMarkers.count(key)) continue;
                    
                    float worldU = marker.Position[0] / (float)params.MapSize;
                    float worldV = marker.Position[2] / (float)params.MapSize;
                    
                    if (spawnIndex == 0) { // Debug first marker
                        char dbg[128];
                        snprintf(dbg, sizeof(dbg), "Pos(%.1f, %.1f) U(%.2f) V(%.2f) MapSz(%d)", marker.Position[0], marker.Position[2], worldU, worldV, params.MapSize);
                        ImGui::GetWindowDrawList()->AddText(ImVec2(p0.x + 10, p0.y + 10), IM_COL32(255,255,255,255), dbg);
                    }
                    
                    float screenU = (worldU - uv0.x) / (uv1.x - uv0.x);
                    float screenV = (worldV - uv0.y) / (uv1.y - uv0.y);
                    
                    ImVec2 screenPos;
                    screenPos.x = p0.x + screenU * renderSize;
                    screenPos.y = p0.y + screenV * renderSize;
                    
                    // Base size is ~32 pixels for zoom 1
                    float baseScale = 32.0f;
                    if (marker.Type == "Alloy" || marker.Type == "Alloys") baseScale *= params.MarkerScaleAlloy;
                    else if (marker.Type == "Spawn" || marker.Type == "Spawns") baseScale *= params.MarkerScaleSpawn;
                    else if (marker.Type == "Plasma" || marker.Type == "Plasmas") baseScale *= params.MarkerScalePlasma;
                    
                    ImVec2 iconP0(screenPos.x - baseScale/2.0f, screenPos.y - baseScale/2.0f);
                    ImVec2 iconP1(screenPos.x + baseScale/2.0f, screenPos.y + baseScale/2.0f);
                    
                    float globalColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                    std::string iconName = marker.Type;
                    
                    if (marker.Type == "Alloy" || marker.Type == "Alloys") {
                        iconName = params.GlobalIconAlloy;
                        globalColor[0] = params.MarkerColorAlloy[0]; globalColor[1] = params.MarkerColorAlloy[1]; globalColor[2] = params.MarkerColorAlloy[2]; globalColor[3] = params.MarkerColorAlloy[3];
                    } else if (marker.Type == "Spawn" || marker.Type == "Spawns") {
                        iconName = params.GlobalIconSpawn;
                        globalColor[0] = params.MarkerColorSpawn[0]; globalColor[1] = params.MarkerColorSpawn[1]; globalColor[2] = params.MarkerColorSpawn[2]; globalColor[3] = params.MarkerColorSpawn[3];
                        
                        // Tint by army color
                        ImU32 sc = spawnColors[spawnIndex % numSpawnColors];
                        globalColor[0] *= ((sc >> 0) & 0xFF) / 255.0f;
                        globalColor[1] *= ((sc >> 8) & 0xFF) / 255.0f;
                        globalColor[2] *= ((sc >> 16) & 0xFF) / 255.0f;
                        globalColor[3] *= ((sc >> 24) & 0xFF) / 255.0f;
                        spawnIndex++;
                    } else if (marker.Type == "Plasma" || marker.Type == "Plasmas") {
                        iconName = params.GlobalIconPlasma;
                        globalColor[0] = params.MarkerColorPlasma[0]; globalColor[1] = params.MarkerColorPlasma[1]; globalColor[2] = params.MarkerColorPlasma[2]; globalColor[3] = params.MarkerColorPlasma[3];
                    }
                    
                    if (!marker.IconOverride.empty()) {
                        iconName = marker.IconOverride;
                    }
                    
                    GLuint tex = GetMarkerIcon(iconName, params);
                    if (tex == 0) tex = GetMarkerIcon(marker.Type, params); // Fallback to base type name
                    if (tex == 0 && !params.IconCache.empty()) tex = params.IconCache.begin()->second; // Ultimate fallback
                    ImU32 tintCol = IM_COL32(
                        std::clamp((int)(globalColor[0] * marker.Color[0] * 255.0f), 0, 255),
                        std::clamp((int)(globalColor[1] * marker.Color[1] * 255.0f), 0, 255),
                        std::clamp((int)(globalColor[2] * marker.Color[2] * 255.0f), 0, 255),
                        std::clamp((int)(globalColor[3] * marker.Color[3] * 255.0f), 0, 255)
                    );
                    
                    // Is the mouse over this marker?
                    bool hit = (mousePos.x >= iconP0.x && mousePos.x <= iconP1.x &&
                                mousePos.y >= iconP0.y && mousePos.y <= iconP1.y);
                                
                    if (hit && isHoveringPreview) {
                        if (ImGui::IsMouseClicked(0) && !isDraggingMarker) {
                            draggingMarker = key;
                            selectedMarkerKey = key;
                            activeTab = 9; // Switch to Markers tab
                            isDraggingMarker = true;
                            // Record relative offset to icon center
                            dragOffset.x = mousePos.x - screenPos.x;
                            dragOffset.y = mousePos.y - screenPos.y;
                        }
                        if (ImGui::IsMouseClicked(1)) {
                            ImGui::OpenPopup(("MarkerContext_" + key).c_str());
                        }
                    }
                    
                    if (tex != 0) {
                        if (!marker.IsValid) {
                            ImGui::GetWindowDrawList()->AddRectFilled(iconP0, iconP1, IM_COL32(255, 0, 0, 150));
                        }
                        ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)tex, iconP0, iconP1, ImVec2(0,0), ImVec2(1,1), tintCol);
                    } else {
                        ImU32 col = IM_COL32(255, 255, 0, 255);
                        if (!marker.IsValid) col = IM_COL32(255, 0, 0, 255);
                        else if (marker.Type == "Spawn") col = tintCol;
                        else if (marker.Type == "Plasma") col = IM_COL32(255, 0, 255, 255);
                        
                        if (marker.Type == "Alloy") {
                            ImVec2 p1(screenPos.x, iconP0.y);
                            ImVec2 p2(iconP1.x, iconP1.y);
                            ImVec2 p3(iconP0.x, iconP1.y);
                            ImGui::GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, col);
                        } else {
                            ImGui::GetWindowDrawList()->AddRectFilled(iconP0, iconP1, col);
                        }
                    }
                    
                    if (ImGui::BeginPopup(("MarkerContext_" + key).c_str())) {
                        if (ImGui::MenuItem("Delete Marker")) {
                            params.MarkersList.erase(key);
                            ImGui::EndPopup();
                            break; // Stop iteration as map was modified
                        }
                        ImGui::EndPopup();
                    }
                }
                
                // Handle dragging
                if (isDraggingMarker) {
                    if (ImGui::IsMouseDragging(0, 0.0f)) {
                        auto it = params.MarkersList.find(draggingMarker);
                        if (it != params.MarkersList.end()) {
                            float screenU_drag = (mousePos.x - dragOffset.x - p0.x) / renderSize;
                            float screenV_drag = (mousePos.y - dragOffset.y - p0.y) / renderSize;
                            
                            float worldU_drag = uv0.x + screenU_drag * (uv1.x - uv0.x);
                            float worldV_drag = uv0.y + screenV_drag * (uv1.y - uv0.y);
                            
                            it->second.Position[0] = std::clamp(worldU_drag * params.MapSize, 0.0f, (float)params.MapSize);
                            it->second.Position[2] = std::clamp(worldV_drag * params.MapSize, 0.0f, (float)params.MapSize);
                        }
                    }
                    if (ImGui::IsMouseReleased(0)) {
                        bNeedsMapUpdate = true; // Trigger JSON save update or logic if necessary upon drop
                        isDraggingMarker = false;
                        draggingMarker = "";
                    }
                }
                
                // Context Menu on the map itself
                if (isHoveringPreview && ImGui::IsMouseClicked(1) && !isDraggingMarker) {
                    ImGui::OpenPopup("AddMarkerMenu");
                }
                
                if (ImGui::BeginPopup("AddMarkerMenu")) {
                    int selIdx = params.SelectedPlacedLayerIndex;
                    if (selIdx >= 0 && selIdx < (int)params.PlacedMarkerLayers.size()) {
                        auto& layer = params.PlacedMarkerLayers[selIdx];
                        if (layer.Type == SanmapGen::LayerType::Manual) {
                            if (ImGui::BeginMenu("Add Marker to Selected Layer")) {
                                auto placeMarker = [&](const std::string& type, const std::string& prefix) {
                                    std::string newKey = prefix + "_" + std::to_string(params.MarkersList.size() + 1);
                                    SanmapGen::MarkerTransform m;
                                    m.Type = type;
                                    m.IsManual = true;
                                    float screenU_clk = (mousePos.x - p0.x) / renderSize;
                                    float screenV_clk = (mousePos.y - p0.y) / renderSize;
                                    m.Position[0] = (uv0.x + screenU_clk * (uv1.x - uv0.x)) * params.MapSize;
                                    m.Position[2] = (uv0.y + screenV_clk * (uv1.y - uv0.y)) * params.MapSize;
                                    m.Scale[0] = 1.0f; m.Scale[1] = 1.0f; m.Scale[2] = 1.0f;
                                    params.MarkersList[newKey] = m;
                                    layer.MarkerKeys.push_back(newKey);
                                    bNeedsMapUpdate = true;
                                    bNeedsPreviewRender = true;
                                };
                                
                                if (ImGui::MenuItem("Alloy")) placeMarker("Alloy", "Alloys");
                                if (ImGui::MenuItem("Plasma")) placeMarker("Plasma", "Plasmas");
                                if (ImGui::MenuItem("Spawn")) placeMarker("Spawn", "Spawns");
                                ImGui::EndMenu();
                            }
                        } else if (layer.Type == SanmapGen::LayerType::Fixed) {
                            ImGui::TextDisabled("Selected layer is Fixed (Imported).");
                            ImGui::TextDisabled("Cannot manually place markers here.");
                        }
                    } else {
                        ImGui::TextDisabled("No Placed Marker Layer selected.");
                        ImGui::TextDisabled("Select a Manual layer in the UI first.");
                    }
                    ImGui::EndPopup();
                }
            }
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

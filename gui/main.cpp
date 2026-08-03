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

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

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
        
        ImGui::Text("Global Settings");
        ImGui::Separator();
        if (ImGui::InputInt("Map Seed", &params.Seed)) bNeedsPreviewUpdate = true;
        
        const int sizes[] = { 256, 512, 1024, 2048, 4096 };
        const char* size_labels[] = { "256", "512", "1024", "2048", "4096" };
        static int size_current_idx = 1; // Default 512
        if (ImGui::BeginCombo("Map Size", size_labels[size_current_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(sizes); n++) {
                const bool is_selected = (size_current_idx == n);
                if (ImGui::Selectable(size_labels[n], is_selected)) {
                    size_current_idx = n;
                    bNeedsPreviewUpdate = true;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        params.MapSize = sizes[size_current_idx];
        
        ImGui::Spacing();
        ImGui::Text("Symmetry Blending Mode");
        ImGui::Separator();
        
        const char* alg_labels[] = { "2D Fold (Hardlines)", "2D Blur (Post-Process)", "2D Cross-Fade", "3D Cylinder", "3D Torus", "Native Hash (X/Z/Point)" };
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
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Cylinder3D) {
            if (ImGui::SliderFloat("Cylinder Stretch", &params.CylinderZScale, 0.1f, 10.0f)) bNeedsPreviewUpdate = true;
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Torus3D) {
            if (ImGui::SliderFloat("Major Radius", &params.TorusMajorRadius, 10.0f, 500.0f)) bNeedsPreviewUpdate = true;
            if (ImGui::SliderFloat("Minor Radius", &params.TorusMinorRadius, 10.0f, 500.0f)) bNeedsPreviewUpdate = true;
        }
        
        ImGui::Spacing();
        ImGui::Text("Water Settings");
        ImGui::Separator();
        if (ImGui::SliderFloat("Water Level Min", &params.WaterLevelMin, 0.0f, 128.0f)) bNeedsPreviewUpdate = true;
        if (ImGui::SliderFloat("Water Level Max", &params.WaterLevelMax, 0.0f, 128.0f)) bNeedsPreviewUpdate = true;
        if (ImGui::SliderFloat("Deep Water Min", &params.DeepWaterDepthMin, 0.0f, 128.0f)) bNeedsPreviewUpdate = true;
        if (ImGui::SliderFloat("Deep Water Max", &params.DeepWaterDepthMax, 0.0f, 128.0f)) bNeedsPreviewUpdate = true;
        
        ImGui::Spacing();
        ImGui::Text("Gameplay");
        ImGui::Separator();
        if (ImGui::SliderInt("Spawn Points", &params.SpawnPointCount, 2, 16)) bNeedsPreviewUpdate = true;
        if (ImGui::SliderFloat("Alloy Multiplier", &params.AlloyMultiplier, 0.0f, 3.0f)) bNeedsPreviewUpdate = true;
        if (ImGui::SliderFloat("Hydro Multiplier", &params.HydroMultiplier, 0.0f, 3.0f)) bNeedsPreviewUpdate = true;
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Dynamic Terrain Layers");
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        if (ImGui::Button("Add Layer")) {
            SanmapGen::NoiseLayer newLayer;
            newLayer.Name = "New Layer " + std::to_string(params.Layers.size() + 1);
            params.Layers.push_back(newLayer);
            bNeedsPreviewUpdate = true;
        }
        ImGui::Separator();
        
        // Draw each layer as a collapsible header
        for (size_t i = 0; i < params.Layers.size(); ++i) {
            auto& layer = params.Layers[i];
            
            ImGui::PushID(i); // Unique ID scope for this layer's UI
            
            if (ImGui::Checkbox("##enabled", &layer.Enabled)) bNeedsPreviewUpdate = true;
            ImGui::SameLine();
            
            bool expanded = ImGui::CollapsingHeader(layer.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            
            if (expanded) {
                // Rename
                char nameBuf[128];
                strncpy(nameBuf, layer.Name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Layer Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {
                    layer.Name = nameBuf;
                }
                
                // Blend Mode
                const char* blend_labels[] = { "Add", "Subtract", "Multiply", "Overlay" };
                if (ImGui::BeginCombo("Blend Mode", blend_labels[(int)layer.Blend])) {
                    for (int n = 0; n < 4; n++) {
                        if (ImGui::Selectable(blend_labels[n], ((int)layer.Blend == n))) {
                            layer.Blend = (SanmapGen::BlendMode)n;
                            bNeedsPreviewUpdate = true;
                        }
                    }
                    ImGui::EndCombo();
                }
                
                // Noise Type
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
                
                // Fractal Type
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
                
                // Symmetry Bitmask (Checkboxes)
                ImGui::Text("Symmetry:");
                ImGui::SameLine();
                bool symPoint = (layer.SymmetryMask & SanmapGen::Symmetry_Point);
                bool symX = (layer.SymmetryMask & SanmapGen::Symmetry_X);
                bool symZ = (layer.SymmetryMask & SanmapGen::Symmetry_Z);
                bool symXY = (layer.SymmetryMask & SanmapGen::Symmetry_XY);
                bool symRadial = (layer.SymmetryMask & SanmapGen::Symmetry_Radial);
                
                if (ImGui::Checkbox("Point", &symPoint)) { layer.SymmetryMask ^= SanmapGen::Symmetry_Point; bNeedsPreviewUpdate = true; }
                ImGui::SameLine();
                if (ImGui::Checkbox("X", &symX)) { layer.SymmetryMask ^= SanmapGen::Symmetry_X; bNeedsPreviewUpdate = true; }
                ImGui::SameLine();
                if (ImGui::Checkbox("Z", &symZ)) { layer.SymmetryMask ^= SanmapGen::Symmetry_Z; bNeedsPreviewUpdate = true; }
                ImGui::SameLine();
                if (ImGui::Checkbox("XY", &symXY)) { layer.SymmetryMask ^= SanmapGen::Symmetry_XY; bNeedsPreviewUpdate = true; }
                ImGui::SameLine();
                if (ImGui::Checkbox("Radial", &symRadial)) { layer.SymmetryMask ^= SanmapGen::Symmetry_Radial; bNeedsPreviewUpdate = true; }
                
                // Noise Sliders
                ImGui::Spacing();
                ImGui::Text("Noise Properties");
                if (ImGui::DragFloat("Frequency (Scale)", &layer.Frequency, 0.001f, 0.0001f, 0.5f, "%.4f")) bNeedsPreviewUpdate = true;
                if (ImGui::SliderInt("Octaves (Roughness)", &layer.Octaves, 1, 10)) bNeedsPreviewUpdate = true;
                if (ImGui::SliderFloat("Gain (Amplitude)", &layer.Gain, 0.1f, 5.0f)) bNeedsPreviewUpdate = true;
                if (layer.Fractal == SanmapGen::FractalType::PingPong) {
                    if (ImGui::SliderFloat("PingPong Strength", &layer.PingPongStrength, 0.1f, 5.0f)) bNeedsPreviewUpdate = true;
                }
                if (ImGui::SliderFloat("Opacity", &layer.Opacity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                
                if (layer.Type == SanmapGen::NoiseType::Cellular) {
                    if (ImGui::SliderFloat("Cellular Jitter", &layer.CellularJitter, 0.0f, 2.0f)) bNeedsPreviewUpdate = true;
                }
                
                // Terrain Density Shaping Sliders
                ImGui::Spacing();
                ImGui::Text("Density Shaping");
                if (ImGui::SliderFloat("Land Density", &layer.LandDensity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                if (ImGui::SliderFloat("Plateau Density", &layer.PlateauDensity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                if (ImGui::SliderFloat("Mountain Density", &layer.MountainDensity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                if (ImGui::SliderFloat("Ramp Density", &layer.RampDensity, 0.0f, 1.0f)) bNeedsPreviewUpdate = true;
                
                ImGui::Spacing();
                if (ImGui::Button("Delete Layer")) {
                    params.Layers.erase(params.Layers.begin() + i);
                    bNeedsPreviewUpdate = true;
                    ImGui::PopID();
                    break; // Break the loop so we don't crash from iterator invalidation
                }
            }
            ImGui::PopID();
            ImGui::Separator();
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
            previewTexture = SanmapGen::PreviewRenderer::UpdatePreviewTexture(dummyMap, previewTexture);
        }
        
        if (ImGui::Button("Final Export (1025 Accurate)", ImVec2(-1, 40)))
        {
            // TODO: Dispatch accurate 1025 non-padded physics generation and write to disk
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

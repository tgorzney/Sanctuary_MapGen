cmake_minimum_required(VERSION 3.24)
project(SanmapGenerator VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use FetchContent to pull dependencies automatically
include(FetchContent)

# 1. Fetch GLFW (Window management and OpenGL context)
# Using URL instead of GIT_REPOSITORY so you don't need Git installed!
FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 2. Fetch Dear ImGui
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/heads/docking.zip
)
FetchContent_MakeAvailable(imgui)

# 3. Fetch nlohmann/json for Sanmap export
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Create a library for ImGui since it doesn't provide its own CMakeLists out of the box
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# Glob all source files in core/ and gui/ automatically
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "core/*.cpp" "core/*.h")
list(FILTER CORE_SOURCES EXCLUDE REGEX "miniz\\.c")
file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS "gui/*.cpp" "gui/*.h")

# Download FastNoiseLite header if it doesn't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/FastNoiseLite.h")
    message(STATUS "Downloading FastNoiseLite.h...")
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/Auburn/FastNoiseLite/master/Cpp/FastNoiseLite.h" 
        "${CMAKE_CURRENT_SOURCE_DIR}/core/FastNoiseLite.h"
        SHOW_PROGRESS)
endif()

# Download stb_image.h if it doesn't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/stb_image.h")
    message(STATUS "Downloading stb_image.h...")
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" 
        "${CMAKE_CURRENT_SOURCE_DIR}/core/stb_image.h"
        SHOW_PROGRESS)
endif()

# Download stb_image_write.h if it doesn't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/stb_image_write.h")
    message(STATUS "Downloading stb_image_write.h...")
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h" 
        "${CMAKE_CURRENT_SOURCE_DIR}/core/stb_image_write.h"
        SHOW_PROGRESS)
endif()

# Fetch miniz (amalgamated version via release zip)
FetchContent_Declare(
    miniz
    URL https://github.com/richgel999/miniz/releases/download/2.2.0/miniz-2.2.0.zip
)
FetchContent_MakeAvailable(miniz)

# Copy the amalgamated miniz files over the empty core/miniz files to fix the include shadows
file(COPY "${miniz_SOURCE_DIR}/miniz.h" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/core/")
file(COPY "${miniz_SOURCE_DIR}/miniz.c" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/core/")

# Find OpenMP
find_package(OpenMP REQUIRED)

# Add our application executable
add_executable(SanmapGenerator
    ${CORE_SOURCES}
    ${GUI_SOURCES}
    core/miniz.c
)

# Include directories
target_include_directories(SanmapGenerator PRIVATE
    core
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${miniz_SOURCE_DIR}
)

# Link libraries
target_link_libraries(SanmapGenerator PRIVATE
    imgui
    glfw
    nlohmann_json::nlohmann_json
    OpenMP::OpenMP_CXX
)

# On Windows, we need to link OpenGL32
if(WIN32)
    target_link_libraries(SanmapGenerator PRIVATE opengl32)
endif()

# Fetch stb
FetchContent_Declare(
    stb
    URL https://github.com/nothings/stb/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(stb)
target_include_directories(SanmapGenerator PRIVATE ${stb_SOURCE_DIR})

# ---------------------------------------------------------------------------
# SanGen v2 (src/) — the ARCH-conformant rebuild: math / data / params / proc /
# pipeline / sys / io. Built as one static library plus one executable per
# acceptance test (Constitution §7). The legacy core/ + gui/ tree above is
# untouched until the M4/M5 UI work-orders replace it.
# ---------------------------------------------------------------------------
enable_testing()

file(GLOB_RECURSE SANGEN_V2_SOURCES CONFIGURE_DEPENDS
    "src/math/*.cpp"     "src/math/*.h"
    "src/data/*.cpp"     "src/data/*.h"
    "src/params/*.cpp"   "src/params/*.h"
    "src/proc/*.cpp"     "src/proc/*.h"
    "src/pipeline/*.cpp" "src/pipeline/*.h"
    "src/ui/*.cpp"       "src/ui/*.h"
    "src/sys/*.cpp"      "src/sys/*.h"
    "src/io/*.cpp"       "src/io/*.h"
)
# Test translation units are compiled into their own executables, never the library. The app's
# entry point is excluded for the same reason: a main() in a static library is only ever pulled in
# by accident, and every acceptance binary declares its own (M5-7).
list(FILTER SANGEN_V2_SOURCES EXCLUDE REGEX "_Test\\.cpp$")
list(FILTER SANGEN_V2_SOURCES EXCLUDE REGEX "ApplicationMain_UI\\.cpp$")

# The GPU twins (ARCH §1.4) are compiled by the driver at run time, not by MSVC — they are
# listed so each .glsl sits beside its CPU .cpp in the IDE and any divergence is visible.
# They live beside their CPU twin in the OWNING layer's folder: the PROC stage kernels in
# src/proc, the preview composite's kernel in src/ui (ARCH §8.1 — the composite is UI).
file(GLOB_RECURSE SANGEN_V2_SHADERS CONFIGURE_DEPENDS "src/proc/*.glsl" "src/ui/*.glsl")
set_source_files_properties(${SANGEN_V2_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(SanGenV2 STATIC ${SANGEN_V2_SOURCES} ${SANGEN_V2_SHADERS})
target_include_directories(SanGenV2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Vendored third-party headers only (FastNoiseLite, fetched by the download rule above).
# This is NOT a dependency on the legacy core/ layer tree — no src/ file includes a core/
# project header. Where vendored headers live once core/ is retired (ARCH §5.5) is an open
# question for the ARCH Expert.
target_include_directories(SanGenV2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)
# miniz is a vendored third-party library that happens to sit in core/ beside FastNoiseLite —
# NOT legacy core/ project code. src/io/SanpackReader_Inflate_IO.cpp is the only translation
# unit that talks to it (ASSET_LOADING_SPEC: the sanpack is a zip), so its amalgamated .c is
# compiled into the v2 library rather than pulled from the legacy executable's object set.
target_sources(SanGenV2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core/miniz.c)
# The M5 widget layer draws through Dear ImGui, so the library needs imgui's HEADERS to compile
# those translation units. A static library only pulls the objects an executable actually
# references, so the imgui-free acceptance tests below still link against SanGenV2 alone even
# though imgui is now linked (below) for the widgets that call real imgui functions (ImGui::Image
# and friends), not just its headers.
target_include_directories(SanGenV2 PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_features(SanGenV2 PUBLIC cxx_std_17)
if(MSVC)
    # /permissive- for conforming lookup; the GPU seam resolves GL entry points itself.
    target_compile_options(SanGenV2 PRIVATE /permissive- /W3)
endif()
if(WIN32)
    target_link_libraries(SanGenV2 PUBLIC opengl32)
endif()
# The UI layer draws through Dear ImGui (UI_FRAMEWORK_SPEC: "imgui is used, but bypassed on the
# hot paths"), so the v2 library links the same imgui target the legacy executable does. PUBLIC:
# a consumer that draws a v2 widget (the app shell, and each widget's acceptance test) needs the
# headers too. imgui itself pulls in glfw for its backends.
target_link_libraries(SanGenV2 PUBLIC imgui)

# The GPU-backed units load their .glsl twins from ONE shader directory at run time
# (Sys::GpuResourceManager resolves `<shaderDirectory>/<fileName>` — a single directory, never an
# absolute path). Their sources deliberately live in the OWNING layer's folder (src/proc for the
# PROC stages, src/ui for the preview composite), so the build stages every kernel into one
# directory the manager can be pointed at. ARCH §1.1/§1.4 give every kernel a layer-suffixed
# name, so the staged directory is collision-free. configure_file registers each source as a
# configure dependency: editing a .glsl re-runs CMake and re-stages it.
set(SANGEN_V2_SHADER_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sangen_shaders")
foreach(shaderFile ${SANGEN_V2_SHADERS})
    get_filename_component(shaderName "${shaderFile}" NAME)
    configure_file("${shaderFile}" "${SANGEN_V2_SHADER_DIRECTORY}/${shaderName}" COPYONLY)
endforeach()

# ---------------------------------------------------------------------------
# The v2 application (M5-7). `Ui::Application` and its aspect translation units live in the library
# with the rest of src/ui; only the entry point is compiled here, so the acceptance tests drive the
# very same shell code the shipped binary runs. imgui + glfw + opengl32 arrive transitively:
# SanGenV2 links imgui PUBLIC (M5-5) and imgui links glfw PUBLIC, so the app declares neither.
# ---------------------------------------------------------------------------
add_executable(SanGenV2App src/ui/ApplicationMain_UI.cpp)
target_link_libraries(SanGenV2App PRIVATE SanGenV2)
set_property(TARGET SanGenV2App PROPERTY FOLDER "SanGenV2")

# The kernels ship BESIDE the executable: ApplicationMain_UI.cpp resolves `<exeDir>/sangen_shaders`
# when no explicit directory is given, so a copied-out build finds its .glsl twins with no absolute
# path anywhere (ARCH §4 / GpuResource_SYS.h "never a hardcoded absolute path"). The staged
# directory above stays the single source: the tests are handed it directly, the app gets a copy.
add_custom_command(TARGET SanGenV2App POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SANGEN_V2_SHADER_DIRECTORY}" "$<TARGET_FILE_DIR:SanGenV2App>/sangen_shaders"
    COMMENT "Staging SanGen v2 GPU kernels beside the application")

# add_sangen_test(<name> <source>...) — one acceptance-test executable, registered
# with CTest and handed the shader directory as argv[1] (ignored by tests that do
# not need a GL context).
function(add_sangen_test testName)
    add_executable(${testName} ${ARGN})
    target_link_libraries(${testName} PRIVATE SanGenV2)
    add_test(NAME ${testName} COMMAND ${testName} "${SANGEN_V2_SHADER_DIRECTORY}")
    set_property(TARGET ${testName} PROPERTY FOLDER "SanGenV2 Tests")
endfunction()

# MATH
add_sangen_test(FloatVector_MATH_Test           src/math/FloatVector_MATH_Test.cpp)
add_sangen_test(JumpFloodDistanceField_MATH_Test src/math/JumpFloodDistanceField_MATH_Test.cpp)
add_sangen_test(Morton_MATH_Test                src/math/Morton_MATH_Test.cpp)
add_sangen_test(RadialClearance_MATH_Test       src/math/RadialClearance_MATH_Test.cpp)
add_sangen_test(Reciprocal_MATH_Test            src/math/Reciprocal_MATH_Test.cpp)
add_sangen_test(Trigonometry_MATH_Test          src/math/Trigonometry_MATH_Test.cpp)

# DATA
add_sangen_test(EntityIdBuffer_DATA_Test        src/data/EntityIdBuffer_DATA_Test.cpp)
add_sangen_test(FloatField_DATA_Test            src/data/FloatField_DATA_Test.cpp)
add_sangen_test(MapFields_DATA_Test             src/data/MapFields_DATA_Test.cpp)
add_sangen_test(SpatialGrid_DATA_Test           src/data/SpatialGrid_DATA_Test.cpp)

# PARAMS
add_sangen_test(Geometry_PARAMS_Test            src/params/Geometry_PARAMS_Test.cpp)
add_sangen_test(GradientRamp_PARAMS_Test        src/params/GradientRamp_PARAMS_Test.cpp)
add_sangen_test(LayerStack_PARAMS_Test          src/params/LayerStack_PARAMS_Test.cpp)
add_sangen_test(MapRecipe_PARAMS_Test           src/params/MapRecipe_PARAMS_Test.cpp)
add_sangen_test(PlacementRules_PARAMS_Test      src/params/PlacementRules_PARAMS_Test.cpp)

# SYS
add_sangen_test(ArenaAllocator_SYS_Test         src/sys/ArenaAllocator_SYS_Test.cpp)
add_sangen_test(Dispatch_SYS_Test               src/sys/Dispatch_SYS_Test.cpp)
add_sangen_test(GpuResource_SYS_Test
    src/sys/GpuResource_SYS_Test.cpp
    src/sys/GpuResource_Texture_SYS_Test.cpp
    src/sys/GpuResource_ShaderPath_SYS_Test.cpp)
add_sangen_test(ThreadPool_SYS_Test             src/sys/ThreadPool_SYS_Test.cpp)
add_sangen_test(AtlasResidency_SYS_Test         src/sys/AtlasResidency_SYS_Test.cpp)

# IO — the M5-4 asset pipeline. One binary whose main() lives in SanpackReader_IO_Test.cpp; the
# synthetic sanpack it runs against is written with the miniz WRITER, so this target (unlike the
# library, which only needs miniz to inflate) also needs the vendored header on its include path.
add_sangen_test(AssetPipeline_IO_Test
    src/io/SanpackReader_IO_Test.cpp
    src/io/AssetAtlasCache_IO_Test.cpp
    src/io/AssetPipeline_TestSupport_IO_Test.cpp
    src/io/AssetPipeline_TestDamage_IO_Test.cpp)
target_include_directories(AssetPipeline_IO_Test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)

# PROC — a stage's checks may span several translation units (ARCH §1.5 ceilings);
# each group below is ONE binary whose main() lives in the first file listed.
add_sangen_test(NoiseBlend_PROC_Test
    src/proc/NoiseBlend_PROC_Test.cpp
    src/proc/NoiseBlend_GpuParity_PROC_Test.cpp
    src/proc/NoiseBlend_GpuBlend_PROC_Test.cpp)
# WO B2 "Scale Features to Map Size". Its own binary rather than a fourth translation unit of the
# group above, so landing it needed no edit to that binary's main(): it drives only the flatten and
# the dirty hash, so it needs neither a GL context nor the parity fixtures those files carry.
add_sangen_test(NoiseBlend_FeatureScale_PROC_Test src/proc/NoiseBlend_FeatureScale_PROC_Test.cpp)
add_sangen_test(Mask_PROC_Test
    src/proc/Mask_PROC_Test.cpp
    src/proc/Mask_Slope_PROC_Test.cpp
    src/proc/Mask_WorldScale_PROC_Test.cpp
    src/proc/Mask_Merge_PROC_Test.cpp
    src/proc/Mask_Purity_PROC_Test.cpp
    src/proc/Mask_DirtyHash_PROC_Test.cpp
    src/proc/Mask_Parity_PROC_Test.cpp)
add_sangen_test(Erosion_PROC_Test               src/proc/Erosion_PROC_Test.cpp)
add_sangen_test(Erosion_Gpu_PROC_Test           src/proc/Erosion_Gpu_PROC_Test.cpp)
add_sangen_test(Thermal_PROC_Test
    src/proc/Thermal_PROC_Test.cpp
    src/proc/Thermal_Params_PROC_Test.cpp
    src/proc/Thermal_Gpu_PROC_Test.cpp)
add_sangen_test(FlowAccumulation_PROC_Test
    src/proc/FlowAccumulation_PROC_Test.cpp
    src/proc/FlowAccumulation_Pipeline_PROC_Test.cpp
    src/proc/FlowAccumulation_Parity_PROC_Test.cpp)
add_sangen_test(Placement_PROC_Test             src/proc/Placement_PROC_Test.cpp)
add_sangen_test(Placement_Symmetry_PROC_Test    src/proc/Placement_Symmetry_PROC_Test.cpp)
add_sangen_test(Placement_Gpu_PROC_Test         src/proc/Placement_Gpu_PROC_Test.cpp)
add_sangen_test(Bake_PROC_Test
    src/proc/Bake_PROC_Test.cpp
    src/proc/Bake_Weights_PROC_Test.cpp
    src/proc/Bake_Gpu_PROC_Test.cpp)

# PIPELINE
add_sangen_test(Generation_PIPELINE_Test          src/pipeline/Generation_PIPELINE_Test.cpp)
add_sangen_test(GenerationAssembler_PIPELINE_Test
    src/pipeline/GenerationAssembler_PIPELINE_Test.cpp
    src/pipeline/GenerationAssembler_Outputs_PIPELINE_Test.cpp
    src/pipeline/GenerationAssembler_Dirty_PIPELINE_Test.cpp)

# UI — each composite acceptance test declares its own main(), so each is its own binary; the
# M4-5 integration test is one binary spanning three translation units.
add_sangen_test(GradientLut_UI_Test               src/ui/GradientLut_UI_Test.cpp)
add_sangen_test(GradientEditorWidget_UI_Test      src/ui/GradientEditorWidget_UI_Test.cpp)
add_sangen_test(IconGridWidget_UI_Test
    src/ui/IconGridWidget_UI_Test.cpp
    src/ui/IconGridWidget_Virtualization_UI_Test.cpp)
add_sangen_test(Picking_UI_Test                   src/ui/Picking_UI_Test.cpp)
add_sangen_test(PreviewComposite_UI_Test          src/ui/PreviewComposite_UI_Test.cpp)
add_sangen_test(PreviewComposite_Wysiwyg_UI_Test  src/ui/PreviewComposite_Wysiwyg_UI_Test.cpp)
add_sangen_test(PreviewComposite_Gpu_UI_Test      src/ui/PreviewComposite_Gpu_UI_Test.cpp)
add_sangen_test(MapCanvas_UI_Test
    src/ui/MapCanvas_UI_Test.cpp
    src/ui/MapCanvas_Render_UI_Test.cpp
    src/ui/MapCanvas_View_UI_Test.cpp
    src/ui/MapCanvas_Picking_UI_Test.cpp)
add_sangen_test(PreviewIntegration_UI_Test
    src/ui/PreviewIntegration_UI_Test.cpp
    src/ui/PreviewIntegration_DirtyTier_UI_Test.cpp
    src/ui/PreviewIntegration_Picking_UI_Test.cpp)

# M5-1 core input widgets. Each test drives the widget's PURE logic (clamping, snapping, the
# press/drag/release state machine) with a synthetic pointer sequence, so none of them needs an
# imgui frame, a window, or a GL context — nor to link imgui. The ImDrawList draw path in the
# matching .cpp is checked by eye against a live frame.
add_sangen_test(RangeSliderWidget_UI_Test      src/ui/RangeSliderWidget_UI_Test.cpp)
add_sangen_test(LabelledDialWidget_UI_Test     src/ui/LabelledDialWidget_UI_Test.cpp)
add_sangen_test(RtToggleWidget_UI_Test         src/ui/RtToggleWidget_UI_Test.cpp)
# The one test that closes the loop on the draw path: it runs the widgets in a real imgui frame
# with synthetic mouse events and inspects the draw data — still no window, no GL, no backend. It
# is therefore the ONLY SanGen test that links imgui. Kept out of add_sangen_test()'s uniform
# recipe on purpose; the final imgui wiring belongs to the app shell (M5-7).
add_sangen_test(CoreInputWidgets_LiveFrame_UI_Test src/ui/CoreInputWidgets_LiveFrame_UI_Test.cpp)
target_link_libraries(CoreInputWidgets_LiveFrame_UI_Test PRIVATE imgui)

# M5-6 parameter tabs. One binary over four translation units: the PARAMS-edit checks per tab and
# the two-tier dirty contract driven through a real GenerationAssembler + PreviewDriver. Every
# check drives the widgets' PURE interaction functions with a synthetic pointer sequence, so the
# test needs no imgui frame, no window and no GL context — the tabs' draw paths are the only part
# that touches imgui, and they are verified by eye against a live frame as the widgets are.
add_sangen_test(ParameterTabs_UI_Test
    src/ui/ParameterTabs_UI_Test.cpp
    src/ui/ParameterTabs_Layers_UI_Test.cpp
    src/ui/ParameterTabs_Rules_UI_Test.cpp
    src/ui/ParameterTabs_DirtyTier_UI_Test.cpp)

# M5-2 list widgets. Both widgets are header-only templates, so the test binary is the only
# translation unit that instantiates them — and ImGuiListClipper plus the drag-drop payload path
# need a live imgui context.
add_sangen_test(ListWidgets_UI_Test
    src/ui/VirtualListWidget_UI_Test.cpp
    src/ui/DraggableListWidget_UI_Test.cpp)
target_link_libraries(ListWidgets_UI_Test PRIVATE imgui)

# M5-7 app shell, headless. One binary over three translation units driving a REAL Ui::Application
# (its default recipe, its injected composite callback, its canvas binding, its atlas bridge) on the
# Cpu twin: no window, no GL, so it asserts the SHELL's assembly rather than a replica of it. The
# two IO translation units are the M5-4 synthetic-sanpack writer, reused so the icon-bridge check
# runs against a genuine zip rather than a stub — which is also why it needs vendored miniz.
add_sangen_test(ApplicationShell_UI_Test
    src/ui/ApplicationShell_UI_Test.cpp
    src/ui/ApplicationShell_DirtyTier_UI_Test.cpp
    src/ui/ApplicationShell_IconBridge_UI_Test.cpp
    src/io/AssetPipeline_TestSupport_IO_Test.cpp
    src/io/AssetPipeline_TestDamage_IO_Test.cpp)
target_include_directories(ApplicationShell_UI_Test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)

# M5-7 launch check. The one SanGen test that opens a real GLFW window and a real GL 4.3 context and
# runs the shell's actual frame loop. Kept non-interactive by creating the window HIDDEN and closing
# it after a fixed frame count; it SKIPS (exit 0, with a message) on a machine that has no context,
# exactly as the GPU parity tests do.
add_sangen_test(ApplicationShell_Window_UI_Test src/ui/ApplicationShell_Window_UI_Test.cpp)

# Tab-rebuild batch A — the shared widget library every rebuilt tab is assembled from. As with the
# M5-1 core inputs, each test drives its widget's PURE logic (clamping, commit/cancel, the collapse
# and picker state machines, the Levels transfer curve) with synthetic input, so none needs an imgui
# frame, a window or a GL context. Each declares its own main(), so each is its own binary.
add_sangen_test(Checkbox_UI_Test         src/ui/Checkbox_UI_Test.cpp)
add_sangen_test(Combo_UI_Test            src/ui/Combo_UI_Test.cpp)
add_sangen_test(SliderScalar_UI_Test     src/ui/SliderScalar_UI_Test.cpp)
add_sangen_test(TextInput_UI_Test        src/ui/TextInput_UI_Test.cpp)
add_sangen_test(ColorSwatch_UI_Test      src/ui/ColorSwatch_UI_Test.cpp)
add_sangen_test(FilePathPicker_UI_Test   src/ui/FilePathPicker_UI_Test.cpp)
add_sangen_test(Levels_UI_Test           src/ui/Levels_UI_Test.cpp)
add_sangen_test(Section_UI_Test          src/ui/Section_UI_Test.cpp)

# Tab-rebuild batch B — the Layer Editor and the Heightmap tab that hosts it. The editor's draw
# path is split across five translation units under the ARCH §1.5 ceilings, but everything the
# tests assert on (the scalar catalogue, the value mirrors, the row actions, the one-frame signal
# order, the soil presets and the gravity bulk write) is PURE, so neither binary needs an imgui
# frame, a window or a GL context. The erosion checks drive a real Pipeline::GenerationAssembler
# (they edit constants reached through it) without ever running it.
add_sangen_test(LayerEditor_UI_Test
    src/ui/LayerEditor_UI_Test.cpp
    src/ui/LayerEditor_Signals_UI_Test.cpp
    src/ui/LayerEditor_Erosion_UI_Test.cpp)
add_sangen_test(HeightmapTab_UI_Test      src/ui/HeightmapTab_UI_Test.cpp)

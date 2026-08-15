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
# Test translation units are compiled into their own executables, never the library.
list(FILTER SANGEN_V2_SOURCES EXCLUDE REGEX "_Test\\.cpp$")

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
target_compile_features(SanGenV2 PUBLIC cxx_std_17)
if(MSVC)
    # /permissive- for conforming lookup; the GPU seam resolves GL entry points itself.
    target_compile_options(SanGenV2 PRIVATE /permissive- /W3)
endif()
if(WIN32)
    target_link_libraries(SanGenV2 PUBLIC opengl32)
endif()

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
add_sangen_test(GpuResource_SYS_Test            src/sys/GpuResource_SYS_Test.cpp)
add_sangen_test(ThreadPool_SYS_Test             src/sys/ThreadPool_SYS_Test.cpp)

# PROC — a stage's checks may span several translation units (ARCH §1.5 ceilings);
# each group below is ONE binary whose main() lives in the first file listed.
add_sangen_test(NoiseBlend_PROC_Test
    src/proc/NoiseBlend_PROC_Test.cpp
    src/proc/NoiseBlend_GpuParity_PROC_Test.cpp
    src/proc/NoiseBlend_GpuBlend_PROC_Test.cpp)
add_sangen_test(Mask_PROC_Test
    src/proc/Mask_PROC_Test.cpp
    src/proc/Mask_Slope_PROC_Test.cpp
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
add_sangen_test(Picking_UI_Test                   src/ui/Picking_UI_Test.cpp)
add_sangen_test(PreviewComposite_UI_Test          src/ui/PreviewComposite_UI_Test.cpp)
add_sangen_test(PreviewComposite_Wysiwyg_UI_Test  src/ui/PreviewComposite_Wysiwyg_UI_Test.cpp)
add_sangen_test(PreviewComposite_Gpu_UI_Test      src/ui/PreviewComposite_Gpu_UI_Test.cpp)
add_sangen_test(PreviewIntegration_UI_Test
    src/ui/PreviewIntegration_UI_Test.cpp
    src/ui/PreviewIntegration_DirtyTier_UI_Test.cpp
    src/ui/PreviewIntegration_Picking_UI_Test.cpp)

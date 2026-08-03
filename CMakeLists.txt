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

# Create a static library for the core generation logic
set(CORE_SOURCES
    core/Mask2D.h
    core/Mask2D.cpp
    core/TerrainGenerator.h
    core/TerrainGenerator.cpp
    core/ErosionSimulator.cpp
    core/ErosionCompute.cpp
    core/TerrainCompute.cpp
)

# Download FastNoiseLite header if it doesn't exist
set(FASTNOISE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/core/FastNoiseLite.h")
if(NOT EXISTS "${FASTNOISE_HEADER}")
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/Auburn/FastNoiseLite/master/Cpp/FastNoiseLite.h" 
        "${FASTNOISE_HEADER}"
        STATUS download_status
    )
endif()

# Add our application executable
add_executable(SanmapGenerator
    gui/main.cpp
    gui/PreviewRenderer.cpp
    ${CORE_SOURCES}
)

# Include directories
target_include_directories(SanmapGenerator PRIVATE
    core
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Link libraries
target_link_libraries(SanmapGenerator PRIVATE
    imgui
    glfw
    nlohmann_json::nlohmann_json
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

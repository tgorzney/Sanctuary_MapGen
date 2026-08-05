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
    core/MapExporter.cpp
    core/FileDialog.cpp
    core/ImageUtils.cpp
    core/PlacementRules.h
    core/PlacementRules.cpp
)

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

# Add our application executable
add_executable(SanmapGenerator
    gui/main.cpp
    gui/PreviewRenderer.cpp
    gui/TerrainTabs.cpp
    gui/MaterialTabs.cpp
    gui/EnvironmentTabs.cpp
    gui/SystemTabs.cpp
    ${CORE_SOURCES}
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

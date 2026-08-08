import os

with open("CMakeLists.txt", "r", encoding="utf-8") as f:
    content = f.read()

# Replace hardcoded CORE_SOURCES with GLOB
old_core_sources = """# Create a static library for the core generation logic
set(CORE_SOURCES
    core/Mask2D.h
    core/Mask2D.cpp
    core/TerrainGenerator.h
    core/TerrainGenerator.cpp
    core/ErosionSimulator.cpp
    core/ErosionCompute.cpp
    core/TerrainCompute.cpp
    core/MapExporter.cpp
    core/MapImporter.cpp
    core/SupComImporter.cpp
    core/FileDialog.cpp
    core/ImageUtils.cpp
    core/PlacementRules.h
    core/PlacementRules.cpp
)"""

new_core_sources = """# Glob all source files in core/ and gui/ automatically
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "core/*.cpp" "core/*.h")
file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS "gui/*.cpp" "gui/*.h")"""

content = content.replace(old_core_sources, new_core_sources)

# Replace executable sources
old_executable = """add_executable(SanmapGenerator
    core/TextureLoader.cpp
    gui/main.cpp
    gui/PreviewRenderer.cpp
    gui/TerrainTabs.cpp
    gui/MaterialTabs.cpp
    gui/EnvironmentTabs.cpp
    gui/SystemTabs.cpp
    ${CORE_SOURCES}
    core/miniz.c
)"""

new_executable = """add_executable(SanmapGenerator
    ${CORE_SOURCES}
    ${GUI_SOURCES}
    core/miniz.c
)"""

content = content.replace(old_executable, new_executable)

# Also ensure miniz is not included twice in CORE_SOURCES
# miniz.c is already explicitly added, we can remove it from glob
fix_glob = """file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "core/*.cpp" "core/*.h")
list(FILTER CORE_SOURCES EXCLUDE REGEX "miniz\\\\.c")"""

content = content.replace('file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "core/*.cpp" "core/*.h")', fix_glob)


with open("CMakeLists.txt", "w", encoding="utf-8") as f:
    f.write(content)

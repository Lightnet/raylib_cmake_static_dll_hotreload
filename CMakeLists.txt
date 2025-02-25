# Minimum CMake version required to build this project
cmake_minimum_required(VERSION 3.10)

# Define the project name and language (C)
project(RaylibExample C)

# Set C standard to C11 and make it required
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configure output directories for all build types
# Ensures binaries, libraries, and archives go to consistent locations in the build directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

# Specific output directories for Debug build type
# Keeps Debug builds organized in a Debug subdirectory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/Debug")

# Disable shared library builds by default (favor static linking unless specified)
# This ensures Raylib and other dependencies are statically linked unless overridden
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Default to static libs" FORCE)

# Toggle between static and DLL modes (hot reload enabled in DLL mode)
# USE_DLL_MODE controls whether the project builds with a dynamic core.dll or static linking
set(USE_DLL_MODE OFF CACHE BOOL "Build with DLL support (hot reload enabled)" FORCE)
message(STATUS "USE_DLL_MODE: ${USE_DLL_MODE}")

# Configure build settings based on USE_DLL_MODE
if (USE_DLL_MODE)
    # Enable DLL mode: Build core as a shared library (core.dll)
    set(BUILD_LIBTYPE_SHARED ON CACHE BOOL "Build core as a shared library" FORCE)
    add_definitions(-DCORE_USE_LIBTYPE_SHARED)  // Define macro for main.c to use DLL logic
    add_definitions(-DBUILD_LIBTYPE_SHARED)    // Define macro for core.c to export DLL functions
else()
    # Static mode: Build core statically into the executable
    set(BUILD_LIBTYPE_SHARED OFF CACHE BOOL "Build core statically" FORCE)
    remove_definitions(-DCORE_USE_LIBTYPE_SHARED)  // Remove DLL-specific macros
    remove_definitions(-DBUILD_LIBTYPE_SHARED)
endif()
message(STATUS "BUILD_LIBTYPE_SHARED: ${BUILD_LIBTYPE_SHARED}")

# Fetch external dependencies (Raylib and Raygui) from GitHub
include(FetchContent)

# Declare Raylib dependency
# Downloads Raylib 5.5 from GitHub and makes it available for the build
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

# Declare Raygui dependency
# Downloads Raygui 4.0 from GitHub for GUI functionality
FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG 4.0
)
FetchContent_MakeAvailable(raygui)

# Define the main executable target (RaylibExample) from main.c
add_executable(${PROJECT_NAME} src/main.c)

# Set include directories for the executable
# Ensures main.c can find headers from project, Raylib, and Raygui
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_SOURCE_DIR}/include          // Custom project includes (if any)
    ${raylib_SOURCE_DIR}/src             // Raylib source headers
    ${raygui_SOURCE_DIR}/src             // Raygui source headers
)

# Configure core library based on build mode
if (USE_DLL_MODE)
    # DLL mode: Build core.c as a shared library (core.dll)
    add_library(core SHARED src/core.c)
    target_link_libraries(core PUBLIC raylib_static)  // Link core.dll with static Raylib
else()
    # Static mode: Build core.c as a static library and link it into the executable
    add_library(core STATIC src/core.c)
    target_link_libraries(${PROJECT_NAME} PUBLIC core)  // Link core into RaylibExample.exe
    target_link_libraries(core PUBLIC raylib_static)    // Link core with static Raylib
endif()

# Set include directories for the core library
# Ensures core.c can find headers from project, Raylib, and Raygui
target_include_directories(core PUBLIC 
    ${CMAKE_SOURCE_DIR}/include          // Custom project includes (if any)
    ${raylib_SOURCE_DIR}/src             // Raylib source headers
    ${raygui_SOURCE_DIR}/src             // Raygui source headers
)

# Set output name for core library (core.dll or core.lib)
set_target_properties(core PROPERTIES OUTPUT_NAME "core")

# Link the executable with static Raylib (always required)
target_link_libraries(${PROJECT_NAME} PUBLIC raylib_static)

# Additional Windows-specific libraries for Raylib
# Required for audio (winmm) and graphics (gdi32)
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC winmm.lib gdi32.lib)
    target_link_libraries(core PUBLIC winmm.lib gdi32.lib)
endif()

# MSVC-specific compile definitions to suppress warnings
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS /wd4267)  // Disable deprecated warnings and size_t conversion warnings
endif()

# Post-build step for DLL mode: Copy core.dll to the runtime directory
# Ensures core.dll is available next to RaylibExample.exe for loading
if (USE_DLL_MODE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/core.dll"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}"
    )
endif()

# Notes:
# - USE_DLL_MODE toggles between static linking (OFF) and DLL hot reloading (ON).
# - BUILD_LIBTYPE_SHARED is set based on USE_DLL_MODE to control core library type.
# - Raylib is built statically (raylib_static) and linked into both core and the executable.
# - Raygui is a single-header library; its implementation is defined in core.c or main.c
#   depending on the mode (static: core.c, DLL: main.c).
# - The shadow copy mechanism in main.c (DLL mode) allows rebuilding core.dll without file locking.
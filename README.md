# raylib cmake static dll hotreload

# License: MIT

# Raylib 5.5
# RayGUI 4.0

# Information:
 Use Grok Beta 3 AI agent to improve the cmake build raylib and raygui example application.

 To have two options to build dll or static. The dll is for hot reload while static is not hot reload.

 Note it use VS2022 build.

 Below is the Grok Beta 3 AI Agent Guide and Set Up. It been clean up to for clean code.

# CMakeLists.txt:

## How It Works:

### Static Mode (USE_DLL_MODE OFF):
 * core.c is compiled into a static library (core.lib), linked into RaylibExample.exe with Raylib (raylib_static).
 * All Raylib and Raygui functions are directly available in the executable.

### DLL Mode (USE_DLL_MODE ON):
 * core.c is compiled into a shared library (core.dll), and main.c loads it dynamically using LoadLibraryExA.
 * Function pointers bridge Raylib/Raygui functions from main.c to core.dll.
 * A shadow copy (core_temp_X.dll) is used to avoid locking core.dll, enabling hot reload.

CMakeLists.txt
```
set(USE_DLL_MODE OFF CACHE BOOL "Build with DLL support (hot reload enabled)" FORCE)
```

# RaylibExample: Static and DLL Hot Reload Setup

## Overview

`RaylibExample` is a C project demonstrating a simple Raylib application with Raygui integration, supporting both static linking and dynamic DLL hot reloading. It features a red background with a clickable "TEST BUTTON" and hot reload capability in DLL mode triggered by pressing 'R'. The project is built with CMake and runs on Windows, leveraging Raylib 5.5 and Raygui 4.0.

### Features
- **Static Mode**: Links `core.c` and Raylib statically into a single executable (`RaylibExample.exe`).
- **DLL Mode**: Builds `core.c` as a dynamic library (`core.dll`), loaded at runtime with shadow copy hot reloading.
- **Hot Reload**: In DLL mode, press 'R' to unload the current DLL, allowing rebuilds of `core.dll`, and automatically reload a new shadow copy after a short delay.
- **Raygui Integration**: Displays a simple GUI button using Raygui’s `GuiButton`.
- **Cross-Mode Compatibility**: Seamlessly switches between static and DLL modes via CMake flags.

## Setup Guide

### Prerequisites
- **CMake**: Version 3.10 or higher.
- **C Compiler**: MSVC (Windows) recommended; GCC/MinGW should work with adjustments.
- **Git**: To fetch Raylib and Raygui dependencies.

### Directory Structure
```
RaylibExample/
├── src/
│   ├── core.c        # Core functionality (drawing, input, DLL exports)
│   ├── main.c        # Entry point and DLL loading logic
│   └── core.h        # Header with function declarations
├── CMakeLists.txt    # Build configuration
└── README.md         # This guide
```

### Build Instructions

1. **Clone or Prepare the Project**
   - Ensure `src/core.c`, `src/main.c`, `src/core.h`, and `CMakeLists.txt` are in your project root.

2. **Configure CMake**
   - Open a terminal in the project root.
   - Create a build directory:
     ```bash
     mkdir build
     cd build
     ```

3. **Build Static Mode (`USE_DLL_MODE OFF`)**
   - Configure and build:
     ```bash
     cmake -S .. -B . -DUSE_DLL_MODE=OFF
     cmake --build . --verbose
     ```
   - Output: `Debug/RaylibExample.exe`

4. **Build DLL Mode (`USE_DLL_MODE ON`)**
   - Configure and build:
     ```bash
     cmake -S .. -B . -DUSE_DLL_MODE=ON
     cmake --build . --verbose
     ```
   - Output: `Debug/RaylibExample.exe` and `Debug/core.dll`

5. **Run**
   - Static Mode:
     ```cmd
     Debug\RaylibExample.exe
     ```
   - DLL Mode:
     ```cmd
     Debug\RaylibExample.exe
     ```
   - In DLL mode, press 'R' to trigger hot reload, rebuild `core.dll`, and wait ~1 second for reload.

### Hot Reload Workflow (DLL Mode)
- Run `RaylibExample.exe`.
- Press 'R' to unload the current shadow DLL (e.g., `core_temp.dll`).
- Rebuild `core.dll` in another terminal:
  ```bash
  cmake --build build --target core
  ```

 * Wait ~1 second; the app reloads a new shadow copy (e.g., core_temp_1.dll).

# Function Pointers Explained:
In DLL mode, main.c dynamically loads core.dll and uses function pointers to bridge Raylib and Raygui functions between the executable and DLL.

## How Pointers Work
 * Loading: LoadLibraryExA loads core.dll’s shadow copy into memory, returning a handle (HINSTANCE).
 * Function Pointers: GetProcAddress retrieves function addresses from core.dll (e.g., core_execute_loop), stored in variables like core_execute_loop_func.
 * Bridging: core_load_raylib_functions passes pointers to Raylib/Raygui functions (e.g., &InitWindow, &GuiButton) from main.c to core.dll, allowing the DLL to call them.

# Example
```
core_load_raylib_functions_func(&InitWindow, &CloseWindow, ..., &GuiButton, &GuiSetStyle);
```
 * &InitWindow: Address of Raylib’s InitWindow function in main.c.
 * core.dll stores these in pointers (e.g., raylib_init_window) and calls them as needed.

# Expanding Features with Raylib/Raygui
 To add more Raylib or Raygui features (e.g., DrawText, GuiLabel):

## Adding to core.c
 1. Static Mode: Directly use new functions (e.g., DrawText):
 ```
 CORE void core_execute_loop() {
    raylib_begin_drawing();
    printf("Drawing frame\n");
    raylib_clear_background(RED);
    DrawText("Hello, Raylib!", 10, 10, 20, WHITE);  // Add Raylib text
    if (raygui_gui_button((Rectangle){ 500, 200, 250, 60 }, "TEST BUTTON")) {
        puts("Button pressed\n");
    }
    raylib_end_drawing();
}
 ```
 2. DLL Mode: Add function pointers in core.h and core.c
 * core.h
```c
#if defined(_WIN32) && defined(BUILD_LIBTYPE_SHARED)
    CORE void core_load_raylib_functions(
        ...,  // Existing parameters
        void (*const in_draw_text)(const char *text, int posX, int posY, int fontSize, Color color)  // Add DrawText
    );
#endif
```
 * core.c
```c
#if defined(_WIN32) && defined(BUILD_LIBTYPE_SHARED)
    void (*raylib_draw_text)(const char *text, int posX, int posY, int fontSize, Color color);
    CORE void core_load_raylib_functions(
        ...,  // Existing parameters
        void (*const in_draw_text)(const char *text, int posX, int posY, int fontSize, Color color)
    ) {
        raylib_draw_text = in_draw_text;
        ...
    }
    CORE void core_execute_loop() {
        raylib_begin_drawing();
        printf("Drawing frame\n");
        raylib_clear_background(RED);
        raylib_draw_text("Hello, Raylib!", 10, 10, 20, WHITE);  // Use pointer
        if (raygui_gui_button((Rectangle){ 500, 200, 250, 60 }, "TEST BUTTON")) {
            puts("Button pressed\n");
        }
        raylib_end_drawing();
    }
#endif
```
 * main.c
```
core_load_raylib_functions_func(&InitWindow, ..., &DrawText);
```
 # Adding to main.c (DLL Mode)
 * Update the call to pass new pointers
 ```c
 core_load_raylib_functions_func(&InitWindow, &CloseWindow, ..., &GuiButton, &GuiSetStyle, &DrawText);
 ```
# Notes on Expansion
 * Raylib Functions: Add any Raylib function (e.g., DrawCircle, PlaySound) by extending the function pointer list in core_load_raylib_functions.
 * Raygui Controls: Add Raygui controls (e.g., GuiLabel) similarly by including them in the pointer list.
 * Synchronization: Ensure main.c and core.c agree on the function pointer signatures to avoid runtime errors.

This setup is now extensible—add features as needed following this pattern!

# Credit:
 * https://medium.com/@TheElkantor/how-to-add-hot-reload-to-your-raylib-proj-in-c-698caa33eb74


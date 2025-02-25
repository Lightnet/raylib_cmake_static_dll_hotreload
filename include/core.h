#pragma once

#include <stdbool.h>
#include "raylib.h"    // Raylib core header for window, drawing, and input functions
#include "raygui.h"    // Raygui header for GUI elements (e.g., GuiButton), included as header-only

// Windows-specific header for DLL mode (BUILD_LIBTYPE_SHARED or CORE_USE_LIBTYPE_SHARED)
// Only included when compiling on Windows and using DLL functionality
#if defined(_WIN32) && (defined(CORE_USE_LIBTYPE_SHARED) || defined(BUILD_LIBTYPE_SHARED))
    #define WIN32_LEAN_AND_MEAN  // Reduces Windows.h bloat by excluding rarely-used APIs
    #define NOGDI               // Excludes GDI (Graphics Device Interface) APIs
    #define NOUSER              // Excludes USER (window management) APIs
    #include <windows.h>        // Required for __declspec and Windows-specific definitions (_WIN32, _M_X64)
#endif

// Define CORE macro for export/import based on build mode
#if defined(_WIN32) && defined(BUILD_LIBTYPE_SHARED)
    #define CORE __declspec(dllexport)  // Export functions from core.dll when building the DLL
#elif defined(_WIN32) && defined(CORE_USE_LIBTYPE_SHARED)
    #define CORE __declspec(dllimport)  // Import functions from core.dll when using the DLL
#else
    #define CORE                        // No decoration for static mode
#endif

// Core functions available in both static and DLL modes
CORE void core_init_window(void);       // Initializes window settings (e.g., FPS)
CORE void core_execute_loop(void);      // Main loop iteration (drawing, input handling)
CORE bool core_window_should_close(void); // Checks if the window should close
CORE void core_exit(void);              // Cleans up and closes the window

// DLL-specific functions (only available when building or using the DLL)
#if defined(_WIN32) && defined(BUILD_LIBTYPE_SHARED)
    // Loads Raylib and Raygui function pointers into the DLL
    CORE void core_load_raylib_functions(
        void (*const in_init_window)(int width, int height, const char* title),  // Initializes window
        void (*const in_close_window)(),                                         // Closes window
        bool (*const in_window_should_close)(),                                  // Checks window closure
        void (*const in_begin_drawing)(),                                        // Begins frame drawing
        void (*const in_end_drawing)(),                                          // Ends frame drawing
        void (*const in_clear_background)(Color color),                          // Clears background
        int (*const in_get_key_pressed)(),                                       // Gets pressed key
        int (*const in_gui_button)(Rectangle bounds, const char *text),          // Draws GUI button
        void (*const in_gui_set_style)(int control, int property, int value)     // Sets GUI style
    );
    CORE void core_get_value_hot_reload(char* out_index);  // Gets hot reload state
#endif
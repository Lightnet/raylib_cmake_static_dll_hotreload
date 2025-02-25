// main.c serves as the entry point for both static and DLL modes
#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // For strcpy, strcat
#if defined(CORE_USE_LIBTYPE_SHARED)
    #include <libloaderapi.h>  // Windows-specific for LoadLibraryExA and GetProcAddress
    #include <windows.h>       // For CopyFileA, Sleep
    #include "raylib.h"        // Raylib header for InitWindow and other functions
    #define RAYGUI_IMPLEMENTATION  // Define Raygui implementation for DLL mode
    #include "raygui.h"        // Include Raygui with implementation
#endif

int main() {
    #if defined(CORE_USE_LIBTYPE_SHARED)
        // DLL mode: Load core.dll dynamically and use function pointers with shadow copy
        printf("DLL mode enabled\n");
        HINSTANCE dll_handle = NULL;  // Handle to the loaded DLL (shadow copy)
        char dll_path[256] = "core_temp.dll";  // Path to the initial shadow copy of core.dll
        void (*core_load_raylib_functions_func)(
            void (*const in_init_window)(int width, int height, const char* title),
            void (*const in_close_window)(),
            bool (*const in_window_should_close)(),
            void (*const in_begin_drawing)(),
            void (*const in_end_drawing)(),
            void (*const in_clear_background)(Color color),
            int (*const in_get_key_pressed)(),
            int (*const in_gui_button)(Rectangle bounds, const char *text),
            void (*const in_gui_set_style)(int control, int property, int value)
        );  // Function pointer to load Raylib/Raygui functions into DLL
        void (*core_init_window_func)(void);         // Setup window
        void (*core_execute_loop_func)();            // Main loop
        void (*core_get_value_hot_reload_func)(char* out_index);  // Hot reload state getter
        void (*core_exit_func)();                    // Cleanup
        bool (*core_window_should_close_func)();     // Window close check
        char activate_hot_reload = 0;                // Hot reload flag in main.c, synced with DLL
        int reload_delay = 0;                        // Delay counter for reload grace period

        // Initialize the window in main.c for DLL mode
        printf("Initializing window in main\n");
        InitWindow(800, 600, "Raylib Example");  // Create 800x600 window
        if (!IsWindowReady()) {
            printf("Window failed to initialize in main!\n");
            exit(1);  // Exit if window creation fails
        }
        printf("Window initialized in main\n");

        // Create initial shadow copy of core.dll
        if (!CopyFileA("core.dll", dll_path, FALSE)) {
            printf("Failed to create shadow copy of core.dll\n");
            exit(1);
        }

        // Load the shadow copy of core.dll
        printf("Loading %s\n", dll_path);
        dll_handle = LoadLibraryExA(dll_path, NULL, 0);  // Load shadow copy
        if (dll_handle != NULL) {
            printf("DLL loaded successfully\n");
            core_load_raylib_functions_func = (void*)GetProcAddress(dll_handle, "core_load_raylib_functions");
            if (NULL == core_load_raylib_functions_func) {
                printf("Can't call core_load_raylib_functions dll function\n");
                exit(1);
            } else {
                printf("Bridging Raylib functions\n");
                // Pass Raylib and Raygui functions to the DLL
                core_load_raylib_functions_func(
                    &InitWindow, &CloseWindow, &WindowShouldClose,
                    &BeginDrawing, &EndDrawing, &ClearBackground,
                    &GetKeyPressed, &GuiButton, &GuiSetStyle
                );
            }
            core_init_window_func = (void*)GetProcAddress(dll_handle, "core_init_window");
            if (NULL == core_init_window_func) {
                printf("Can't call core_init_window dll function\n");
                exit(1);
            } else {
                printf("Calling core_init_window\n");
                core_init_window_func();  // Set up window properties
            }
            core_execute_loop_func = (void*)GetProcAddress(dll_handle, "core_execute_loop");
            if (NULL == core_execute_loop_func) {
                printf("Can't call core_execute_loop dll function\n");
                exit(1);
            }
            core_get_value_hot_reload_func = (void*)GetProcAddress(dll_handle, "core_get_value_hot_reload");
            if (NULL == core_get_value_hot_reload_func) {
                printf("Can't call core_get_value_hot_reload dll function\n");
                exit(1);
            }
            core_exit_func = (void*)GetProcAddress(dll_handle, "core_exit");
            if (NULL == core_exit_func) {
                printf("Can't call core_exit dll function\n");
                exit(1);
            }
            core_window_should_close_func = (void*)GetProcAddress(dll_handle, "core_window_should_close");
            if (NULL == core_window_should_close_func) {
                printf("Can't call core_window_should_close dll function\n");
                exit(1);
            }
        } else {
            printf("Can't load %s\n", dll_path);
            exit(1);
        }

        char temp_dll_path[256];  // Buffer for new temp DLL path
        int reload_count = 0;     // Counter for unique temp DLL names
    #else
        // Static mode: Directly call core functions linked into the executable
        printf("Static mode (no reload)\n");
        InitWindow(800, 600, "Raylib Example");  // Initialize window
        if (!IsWindowReady()) {
            printf("Window failed to initialize in static mode!\n");
            exit(1);
        }
        printf("Window initialized in static mode\n");
        core_init_window();  // Set up window properties
    #endif

    printf("Entering main loop\n");
    int loop_count = 0;
    while (1) {
        #if defined(CORE_USE_LIBTYPE_SHARED)
            core_execute_loop_func();  // Call DLL’s loop function
            //printf("Loop iteration %d\n", ++loop_count);

            // Check hot reload state from DLL and update local flag
            char hot_reload_state = 0;
            core_get_value_hot_reload_func(&hot_reload_state);  // Fetch reload state
            if (hot_reload_state == 1 && activate_hot_reload == 0) {
                activate_hot_reload = 1;  // Sync local flag with DLL state
                reload_delay = 60;        // Set delay (e.g., 60 frames ~1 sec at 60 FPS)
                printf("Hot reload triggered, waiting for rebuild...\n");
            }

            if (core_window_should_close_func && core_window_should_close_func()) {
                break;  // Exit loop if window should close
            }

            // Handle hot reload with delay instead of blocking
            if (activate_hot_reload == 1) {
                if (reload_delay > 0) {
                    reload_delay--;  // Decrement delay counter
                    continue;        // Keep loop running to avoid freeze
                }

                FreeLibrary(dll_handle);  // Unload current shadow DLL
                printf("Unloaded %s\n", dll_path);

                // Create a new unique shadow copy (e.g., core_temp_1.dll, core_temp_2.dll)
                sprintf(temp_dll_path, "core_temp_%d.dll", ++reload_count);
                if (!CopyFileA("core.dll", temp_dll_path, FALSE)) {
                    printf("Failed to create new shadow copy %s\n", temp_dll_path);
                    exit(1);
                }
                strcpy(dll_path, temp_dll_path);  // Update dll_path to new temp file

                Sleep(1000);  // Wait 1 second for rebuild (adjust as needed)

                // Load the new shadow copy
                printf("Loading %s\n", dll_path);
                dll_handle = LoadLibraryExA(dll_path, NULL, 0);
                if (NULL != dll_handle) {
                    core_load_raylib_functions_func = (void*)GetProcAddress(dll_handle, "core_load_raylib_functions");
                    if (NULL == core_load_raylib_functions_func) {
                        printf("Can't call core_load_raylib_functions dll function\n");
                        exit(1);
                    } else {
                        core_load_raylib_functions_func(
                            &InitWindow, &CloseWindow, &WindowShouldClose,
                            &BeginDrawing, &EndDrawing, &ClearBackground,
                            &GetKeyPressed, &GuiButton, &GuiSetStyle
                        );
                    }
                    core_execute_loop_func = (void*)GetProcAddress(dll_handle, "core_execute_loop");
                    if (NULL == core_execute_loop_func) {
                        printf("Can't call core_execute_loop dll function\n");
                        exit(1);
                    }
                    core_get_value_hot_reload_func = (void*)GetProcAddress(dll_handle, "core_get_value_hot_reload");
                    if (NULL == core_get_value_hot_reload_func) {
                        printf("Can't call core_get_value_hot_reload dll function\n");
                        exit(1);
                    }
                    core_exit_func = (void*)GetProcAddress(dll_handle, "core_exit");
                    if (NULL == core_exit_func) {
                        printf("Can't call core_exit dll function\n");
                        exit(1);
                    }
                    core_window_should_close_func = (void*)GetProcAddress(dll_handle, "core_window_should_close");
                    if (NULL == core_window_should_close_func) {
                        printf("Can't call core_window_should_close dll function\n");
                        exit(1);
                    }
                    printf("Reloaded %s successfully\n", dll_path);
                } else {
                    printf("Can't load %s\n", dll_path);
                    exit(1);
                }
                activate_hot_reload = 0;  // Reset reload flag
            }
        #else
            core_execute_loop();  // Call static loop function
            //printf("Loop iteration %d\n", ++loop_count);
            if (core_window_should_close()) {
                break;  // Exit loop if window should close
            }
        #endif
    }

    #if defined(CORE_USE_LIBTYPE_SHARED)
        core_exit_func();  // Call DLL’s cleanup
        FreeLibrary(dll_handle);  // Free DLL handle
        DeleteFileA(dll_path);  // Clean up the last shadow copy
    #else
        core_exit();  // Call static cleanup
    #endif

    printf("Exiting program\n");
    return 0;
}
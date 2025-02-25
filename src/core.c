// core.c implements core functionality for both static and DLL modes
#include "core.h"

// Raygui implementation is defined only in static mode here
// In DLL mode, main.c provides the implementation for function pointers
#if !defined(CORE_USE_LIBTYPE_SHARED)
    #define RAYGUI_IMPLEMENTATION  // Define Raygui implementation for static mode
#endif
#include "raygui.h"  // Include Raygui after defining implementation (if applicable)

// DLL-specific function pointers and logic
#if defined(_WIN32) && defined(BUILD_LIBTYPE_SHARED)
    // Function pointers for Raylib and Raygui functions, loaded dynamically in DLL mode
    void (*raylib_init_window)(int width, int height, const char* title);
    void (*raylib_close_window)();
    bool (*raylib_window_should_close)();
    void (*raylib_begin_drawing)();
    void (*raylib_end_drawing)();
    void (*raylib_clear_background)(Color color);
    int (*raylib_get_key_pressed)();
    int (*raygui_gui_button)(Rectangle bounds, const char *text);
    void (*raygui_gui_set_style)(int control, int property, int value);

    // Loads Raylib and Raygui functions from main.c into the DLL
    CORE void core_load_raylib_functions(
        void (*const in_init_window)(int width, int height, const char* title),
        void (*const in_close_window)(),
        bool (*const in_window_should_close)(),
        void (*const in_begin_drawing)(),
        void (*const in_end_drawing)(),
        void (*const in_clear_background)(Color color),
        int (*const in_get_key_pressed)(),
        int (*const in_gui_button)(Rectangle bounds, const char *text),
        void (*const in_gui_set_style)(int control, int property, int value)
    ) {
        printf("Loading Raylib functions into DLL\n");
        raylib_init_window = in_init_window;         // Assign window init function
        raylib_close_window = in_close_window;       // Assign window close function
        raylib_window_should_close = in_window_should_close;  // Assign close check
        raylib_begin_drawing = in_begin_drawing;     // Assign drawing start
        raylib_end_drawing = in_end_drawing;         // Assign drawing end
        raylib_clear_background = in_clear_background; // Assign background clear
        raylib_get_key_pressed = in_get_key_pressed; // Assign key input
        raygui_gui_button = in_gui_button;           // Assign GUI button
        raygui_gui_set_style = in_gui_set_style;     // Assign GUI style setter
    }

    // Hot reload state variable and getter (DLL-specific)
    char core_active_hot_reload = 0;  // Tracks if 'R' was pressed for reload
    CORE void core_get_value_hot_reload(char* out_index) {
        //printf("core_get_value_hot_reload called\n");
        *out_index = core_active_hot_reload;  // Returns reload state
    }
#else
    // Static mode: Map Raylib/Raygui functions directly to their implementations
    #define raylib_init_window InitWindow
    #define raylib_close_window CloseWindow
    #define raylib_window_should_close WindowShouldClose
    #define raylib_begin_drawing BeginDrawing
    #define raylib_end_drawing EndDrawing
    #define raylib_clear_background ClearBackground
    #define raylib_get_key_pressed GetKeyPressed
    #define raygui_gui_button GuiButton
    #define raygui_gui_set_style GuiSetStyle
#endif

// Initializes window settings (called after InitWindow in main.c)
CORE void core_init_window(void) {
    printf("Setting up window\n");
    if (!IsWindowReady()) {
        printf("Warning: Window not fully ready, skipping setup\n");
        return;  // Skip setup if window isn’t initialized
    }
    printf("Window is ready\n");
    SetTargetFPS(60);  // Set frame rate to 60 FPS (safe Raylib call)
    printf("FPS set to 60\n");
    printf("Window setup complete\n");
}

// Main loop iteration: handles input and drawing
CORE void core_execute_loop() {
    int key_pressed = raylib_get_key_pressed();  // Get any pressed key
    if (key_pressed != 0) {
        printf("Key pressed: %d\n", key_pressed);  // Log key code
    }

    #if defined(_WIN32) && defined(BUILD_LIBTYPE_SHARED)
        if (key_pressed == KEY_R) {  // Check for 'R' key (Raylib key code 82)
            core_active_hot_reload = 1;  // Set reload flag for DLL mode
            printf("Key pressed 'R' for reload ...\n");
        }
    #else
        if (key_pressed == KEY_R) {  // Check for 'R' key in static mode
            printf("Key pressed 'R' in static mode (no reload)\n");
        }
    #endif

    raylib_begin_drawing();  // Start drawing frame (Raylib)
    //printf("Drawing frame\n");
    raylib_clear_background(RED);  // Clear background with red (Raylib)
    if (raygui_gui_button((Rectangle){ 500, 200, 250, 60 }, "TEST BUTTON ASD")) {  // Draw button (Raygui)
        puts("Button pressed\n");  // Log button press
    }
    raylib_end_drawing();  // End drawing frame (Raylib)
}

// Checks if the window should close (e.g., close button clicked)
CORE bool core_window_should_close() {
    bool should_close = raylib_window_should_close();
    //printf("Window should close: %d\n", should_close);
    return should_close;
}

// Cleanup function to close the window
CORE void core_exit() {
    printf("Closing window\n");
    raylib_close_window();  // Close the window (Raylib)
}
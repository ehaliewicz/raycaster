
#include "raylib.h"
#include "stdio.h"

void exit_funker();


void (*exit_func)(void) = 0;

int atexit(void (*f)(void)) {
  //__on_failure (f, 0);
  exit_func = f;
  return 0;
}


int WinMainCRTStartup(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;
    
    //debug_printf("is this running wtf\n");
    
    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    while (!WindowShouldClose()) {   // Detect window close button or ESC key
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    

    if(exit_func != 0) {
        exit_func();
    }
    exit_funker(0);
}
#ifndef PLATFORM_WEB_H
#define PLATFORM_WEB_H

#include "common.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <stddef.h>

typedef enum {
    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_I = 73,
    KEY_K = 75,
    KEY_L = 76,
    KEY_O = 79,
    KEY_P = 80,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Z = 90,
    KEY_SHIFT    = 1016,
    KEY_LSHIFT   = 1016,
    KEY_RSHIFT   = 1017,
    KEY_CONTROL  = 1018,
    KEY_LCONTROL = 1018,
    KEY_RCONTROL = 1019,
    KEY_SPACE = 32,
    KEY_ENTER = 13,
    KEY_LEFT  = 37,
    KEY_RIGHT = 39,
    KEY_UP    = 38,
    KEY_DOWN  = 40,
    KEY_KP_0  = 96,
    KEY_KP_9  = 105,
    MOUSE_BUTTON_LEFT   = 2000,
    MOUSE_BUTTON_MIDDLE = 2001,
    MOUSE_BUTTON_RIGHT  = 2002,
} key;

#define KEY_TABLE_SIZE 2003

/* --- Input --- */
int     platform_is_key_down(int key);
int     platform_is_key_pressed(int key);
int     platform_get_key_pressed();
int     platform_is_mouse_button_pressed(int button);
Vector2 platform_get_mouse_delta();
Vector2 platform_get_mouse_position();
void    platform_set_mouse_position(int x, int y);
void    platform_show_cursor();
void    platform_hide_cursor();

/* --- Window --- */
void platform_init_window(int width, int height, const char *title);
void platform_set_window_size(int width, int height);
void platform_set_vsync(int enabled);
void platform_set_fullscreen();
void platform_set_windowed();
int  platform_is_window_focused();
int  platform_window_should_close();
void platform_begin_frame();
void platform_end_frame();

/* --- Timing --- */
float platform_get_time();
float platform_get_frame_time();

/* --- Drawing ---
   Call platform_update_texture() with your RGBA pixel buffer each frame,
   then platform_end_drawing() blits it to the canvas. */
void platform_begin_drawing();
void platform_end_drawing();
void platform_draw_text(const char *text, Vector2 position, float fontSize, float spacing, u32 tint);

/* --- Textures ---
   CPU-side RGBA buffers on web. platform_update_texture() stores your pixels.
   platform_end_drawing() pushes the last-updated buffer to the canvas.
   platform_draw_texture() is a no-op: the buffer IS the framebuffer. */
void          platform_release_textures();
unsigned int *platform_create_textures(int width, int height);
void          platform_update_texture(unsigned int tex, void *pixels, int width, int height);
void          platform_draw_texture(unsigned int tex, Vector2 pos, float rotation, float scale, int w, int h);

/* --- File I/O --- */
int  platform_save_file_data(const char *path, void *data, size_t num_bytes);
u8  *platform_load_file_data(const char *path, int *out_size);
u8  *platform_load_image(const char *path, int expected_width, int expected_height);
void platform_unload_image(u8 *img_data);

/* --- Thread pool stubs (runs synchronously on web) --- */
#define MAX_JOB_SLOTS 16

typedef struct { void (*fp)(void *); void *args; } thread_func_and_args;
typedef struct { int in_use; void *work; }          work_handle;
typedef struct {
    void                  *threadpool;
    thread_func_and_args   job_slots[MAX_JOB_SLOTS];
    work_handle            work_handles[MAX_JOB_SLOTS];
} jobpool;

jobpool *platform_init_threadpool(int num_threads);
void     platform_add_task(jobpool *jp, void (*fp)(void *arg), void *arg_ptr);
void     platform_join_threadpool(jobpool *jp);

#endif
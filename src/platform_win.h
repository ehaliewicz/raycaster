#ifndef PLATFORM_WIN_H
#define PLATFORM_WIN_H

#include "common.h"
//#include "raylib.h"
#include <windows.h>




typedef enum {
    KEY_A = 0x41,
    KEY_B = 0x42,
    KEY_C = 0x43,
    KEY_D = 0x44,
    KEY_E = 0x45,
    KEY_F = 0x46,
    KEY_I = 0x49,
    KEY_K = 0x4B,
    KEY_L = 0x4C,
    KEY_O = 0x4F,
    KEY_P = 0x50,
    KEY_R = 0x52,
    KEY_S = 0x53,
    KEY_T = 0x54,
    KEY_U = 0x55,
    KEY_V = 0x56,
    KEY_W = 0x57,
    KEY_X = 0x58,
    KEY_Z = 0x5A,
    KEY_SHIFT = VK_SHIFT,
    KEY_LSHIFT = VK_LSHIFT,
    KEY_RSHIFT = VK_RSHIFT,
    KEY_CONTROL = VK_CONTROL,
    KEY_LCONTROL = VK_LCONTROL,
    KEY_RCONTROL = VK_RCONTROL,
    KEY_SPACE = VK_SPACE,
    KEY_ENTER = VK_RETURN,
    KEY_LEFT = VK_LEFT,
    KEY_RIGHT = VK_RIGHT,
    KEY_UP = VK_UP,
    KEY_DOWN = VK_DOWN,
    KEY_KP_0 = VK_NUMPAD0,
    KEY_KP_9 = VK_NUMPAD9,
    MOUSE_BUTTON_LEFT = VK_LBUTTON,
    MOUSE_BUTTON_MIDDLE = VK_MBUTTON,
    MOUSE_BUTTON_RIGHT = VK_RBUTTON
} key;


int platform_is_key_down(int key);
int platform_is_key_pressed(int key);
int platform_get_key_pressed();


void platform_draw_text(const char *text, Vector2 position, float fontSize, float spacing, u32 tint);
int platform_window_should_close();
void platform_begin_frame();
void platform_end_frame();

Vector2 platform_get_mouse_delta();
void platform_show_cursor();
void platform_hide_cursor();
int platform_is_mouse_button_pressed(int button);
Vector2 platform_get_mouse_position();
void platform_set_mouse_position(int x, int y);

void platform_begin_drawing();
void platform_end_drawing();
int platform_is_window_focused();
void platform_init_window(int res_h, int res_v, const char* title);
void platform_set_window_size(int res_w, int res_h);
void platform_set_vsync(int enabled);
void platform_set_fullscreen();
void platform_set_windowed();

float platform_get_time();
float platform_get_frame_time();

int platform_save_file_data(const char* file, void* data, size_t num_bytes);
u8* platform_load_file_data(const char* file, int* out_loaded_bytes);


void platform_release_texture(int tex);
unsigned int platform_create_texture(int width, int height);
void platform_update_texture(unsigned int tex, void *pixels, int xoff, int yoff, int width, int height);
void platform_draw_segment(
    unsigned int tex,
    int seg_idx,
    float attributes[7*3],
    float offsets[4],
    float scales[4]);
void platform_draw_texture(unsigned int tex, Vector2 pos, float rotation, float scale, int w, int h);

u8* platform_load_image(const char* file, int expected_width, int expected_height);
void platform_unload_image(u8* img_data);

void platform_play_sound(const char* sound_path);



#define MAX_JOB_SLOTS 16

typedef struct {
    void (*fp)(void*);
    void* args;
} thread_func_and_args;

typedef struct {
    int in_use;
    void* work;
} work_handle;

typedef struct {
    void* threadpool;
    thread_func_and_args job_slots[MAX_JOB_SLOTS];
    work_handle work_handles[MAX_JOB_SLOTS];
} jobpool;


jobpool* platform_init_threadpool(int num_threads);

void platform_add_task(jobpool* jp,void (*fp)(void* arg), void* arg_ptr);
void platform_join_threadpool(jobpool* jp);

#endif 
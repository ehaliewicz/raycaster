#include "assert.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>

#include "common.h"
#include "my_defs.h"
#include "platform_win.h"

static HWND  g_hwnd;
static HDC   g_dc;
static HGLRC g_rc;
static int   g_running = 1;

#define UNIMPLEMENTED debug_printf("%s is unimplemented\n", __FUNCTION__);

static int *g_keys;
static int *g_keys_prev;
static int g_last_key_pressed;

int platform_is_key_down(int key) {
    return g_keys[key];
}

int platform_is_key_pressed(int key) {
    return g_keys[key] && !g_keys_prev[key];
}

int platform_get_key_pressed() {
    int res = g_last_key_pressed;
    g_last_key_pressed = 0;
    return res;
}

void clear_keys() {
    g_keys = memset(g_keys, 0, sizeof(int)*256);
    g_keys_prev = memset(g_keys, 0, sizeof(int)*256);
}

int platform_is_mouse_button_pressed(int button) {
    return platform_is_key_pressed(button);
}

void platform_draw_text(const char *text, Vector2 position, float fontSize, float spacing, u32 tint) {
    UNIMPLEMENTED
}

static int g_mouse_dx, g_mouse_dy;
Vector2 platform_get_mouse_delta() {
    //*dx = g_mouse_dx;
    //*dy = g_mouse_dy;
    //g_mouse_dx = g_mouse_dy = 0;
    return (Vector2){.x = g_mouse_dx, .y = g_mouse_dy};
}

void platform_show_cursor() {
    while (ShowCursor(1) < 0);
}

void platform_hide_cursor() {
    while (ShowCursor(0) >= 0);
}

Vector2 platform_get_mouse_position() {
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(g_hwnd, &p);
    return (Vector2){.x = p.x, .y = p.y};
}

void platform_set_mouse_position(int x, int y) {
    POINT p = { x, y };
    ClientToScreen(g_hwnd, &p);
    SetCursorPos(p.x, p.y);
}

static LARGE_INTEGER g_freq;
static LARGE_INTEGER g_start;

void platform_init_time(void) {
    QueryPerformanceFrequency(&g_freq);
    QueryPerformanceCounter(&g_start);
}

float platform_get_time() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (float)(now.QuadPart - g_start.QuadPart) / (float)g_freq.QuadPart;
}


int platform_window_should_close() {
    if(g_keys[VK_ESCAPE]) { g_running = 0; }
    my_memcpy(g_keys_prev, g_keys, sizeof(int)*256);
    g_mouse_dx = 0; g_mouse_dy = 0;
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return !g_running;
}

static int g_screen_w, g_screen_h;

void update_viewport(int width, int height) {
    g_screen_w = width;
    g_screen_h = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CLOSE:   g_running = 0; return 0;
        case WM_KEYDOWN: g_keys[wp] = 1; g_last_key_pressed = wp; return 0;
        case WM_KEYUP:   g_keys[wp] = 0; return 0;
        case WM_LBUTTONDOWN: g_keys[VK_LBUTTON] = 1; return 0;
        case WM_LBUTTONUP:   g_keys[VK_LBUTTON] = 0; return 0;
        case WM_RBUTTONDOWN: g_keys[VK_RBUTTON] = 1; return 0;
        case WM_RBUTTONUP:   g_keys[VK_RBUTTON] = 0; return 0;
        case WM_MBUTTONDOWN: g_keys[VK_MBUTTON] = 1; return 0;
        case WM_MBUTTONUP:   g_keys[VK_MBUTTON] = 0; return 0;

        case WM_INPUT: {
            RAWINPUT raw;
            UINT size = sizeof(raw);
            GetRawInputData((HRAWINPUT)lp, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
            if (raw.header.dwType == RIM_TYPEMOUSE) {
                g_mouse_dx += raw.data.mouse.lLastX;
                g_mouse_dy += raw.data.mouse.lLastY;
            }
    return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}


typedef long long int (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

void platform_init_window(int width, int height, const char *title) {
    
    SetProcessDPIAware();

    if(g_keys == NULL) {
        debug_printf("allocating...\n");
        g_keys = my_calloc(sizeof(int)*256, "key buffer");
    }
    if(g_keys_prev == NULL) {
        g_keys_prev = my_calloc(sizeof(int)*256, "key buffer");
    }

    HINSTANCE hInst = GetModuleHandleA(NULL);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = hInst;
    wc.lpszClassName = "game";
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    RECT r = { 0, 0, width, height };
    g_hwnd = CreateWindowA("game", title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInst, NULL);
    platform_set_windowed();

    g_dc = GetDC(g_hwnd);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    SetPixelFormat(g_dc, ChoosePixelFormat(g_dc, &pfd), &pfd);

    g_rc = wglCreateContext(g_dc);
    wglMakeCurrent(g_dc, g_rc);
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)(void*)wglGetProcAddress("wglSwapIntervalEXT");


    update_viewport(width, height);

    glEnable(GL_TEXTURE_2D);

    ShowWindow(g_hwnd, SW_SHOW);
    RAWINPUTDEVICE rid = {0};
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x02;
    rid.hwndTarget  = g_hwnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    platform_init_time();
}

void platform_set_vsync(int enabled) {
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(enabled ? 1 : 0);
    }
}

int g_fullscreen = 0;
void platform_set_windowed() {
    g_fullscreen = 0;
    SetWindowLongA(g_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    RECT r = { 0, 0, g_screen_w, g_screen_h };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    SetWindowPos(g_hwnd, NULL, 100, 100,
        r.right - r.left, r.bottom - r.top,
        SWP_FRAMECHANGED | SWP_NOZORDER);
    update_viewport(g_screen_w, g_screen_h);
}
void platform_set_fullscreen() {
    g_fullscreen = 1;
    SetWindowLongA(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    RECT r = { 0, 0, g_screen_w, g_screen_h };
    SetWindowPos(g_hwnd, HWND_TOP, 0, 0, 
        g_screen_w, g_screen_h, 
        SWP_FRAMECHANGED);
    update_viewport(g_screen_w, g_screen_h);
}

void platform_set_window_size(int width, int height) {
    g_screen_w = width;
    g_screen_h = height;
    if(g_fullscreen) {
        platform_set_fullscreen();
    } else {
        platform_set_windowed();
    }
}
int prev_was_unfocused = 0;
int platform_is_window_focused() {
    if(GetForegroundWindow() != g_hwnd) {
        prev_was_unfocused = 1;
    } else {
        ///if(prev_was_unfocused) {
        ///    clear_keys();
        ///}
        prev_was_unfocused = 0;
    }
    return GetForegroundWindow() == g_hwnd;
}


unsigned int platform_create_texture(int width, int height) {
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
    return (int)tex;
}

void platform_update_texture(unsigned int tex, void *pixels, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
}


void platform_draw_texture(unsigned int tex, Vector2 pos, float rotation, float scale, int w, int h) {

    glBindTexture(GL_TEXTURE_2D, tex);
    glLoadIdentity();
    //glPushMatrix();
        glTranslatef(pos.x, pos.y, 0.0f);
        glRotatef(rotation, 0.0f, 0.0f, 1.0f);
        glScalef(scale, scale, 1.0f);
        // centered on pos
        float half_width = w * 0.5f;
        float half_height = h * 0.5f;
        // uv 0,0 is bottom left
        glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(-half_width, -half_height);
            glTexCoord2f(1, 0); glVertex2f( half_width, -half_height);
            glTexCoord2f(1, 1); glVertex2f( half_width,  half_height);
            glTexCoord2f(0, 1); glVertex2f(-half_width,  half_height);
        glEnd();
    //glPopMatrix();
}



float start_time, end_time;
void platform_begin_drawing() {
    start_time = platform_get_time();
}

void platform_end_drawing() {
    end_time = platform_get_time();
    // TODO: need to handle timing and stuff here
    SwapBuffers(g_dc);
}

float platform_get_frame_time() {
    return end_time-start_time;
}

int platform_save_file_data(const char* path, void* data, size_t size) {
    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written;
    WriteFile(f, data, size, &written, NULL);
    CloseHandle(f);
    return written;
}

u8 *platform_load_file_data(const char *path, int *out_size) {
    HANDLE f = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) { 
        debug_printf("error loading file %s\n", path); 
        *out_size = 0;
         return NULL; 
    }
    
    DWORD size = GetFileSize(f, NULL);
    u8 *data = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    DWORD bytes_read;
    ReadFile(f, data, size, &bytes_read, NULL);
    CloseHandle(f);
    
    *out_size = size;
    return data;
}

u8* platform_load_image(const char* path, int expected_width, int expected_height) {
    int size;
    u8* data = platform_load_file_data(path, &size);
    if (!data) { debug_printf("erro loading file?\n");  return NULL; }
    
    // parse header from data directly, no separate ReadFile calls
    int w   = data[12] | (data[13] << 8);
    int h   = data[14] | (data[15] << 8);
    if(w != expected_width) { debug_printf("Expected width of %i but got %i\n", expected_width, w); return NULL; }
    if(h != expected_height) { debug_printf("Expected height of %i but got %i\n", expected_height, h); return NULL; }
    int bpp = data[16] / 8;
    int flip = !(data[17] & 0x20);


    u8* src = data + 18; // pixel data starts after header
    return src;
    
    // ... rest of conversion as before ...
    
    //platform_free_file_data(data);
    //return t;
}

//void platform_unload_image(u8* img_data) {
//    //free(img_data);
//    VirtualFree(img_data, 0, MEM_RELEASE);
//}

typedef struct thread_pool_
{
    TP_CALLBACK_ENVIRON callback_environ;
    PTP_CLEANUP_GROUP cleanup_group;
    PTP_POOL pool;
} thread_pool;


static thread_pool* thread_pool_create_inner(int cpu_threads)
{
    assert(cpu_threads > 0);
    thread_pool* tp = (thread_pool*)calloc(1, sizeof(thread_pool));

    InitializeThreadpoolEnvironment(&tp->callback_environ);

    tp->pool = CreateThreadpool(NULL);

    SetThreadpoolThreadMinimum(tp->pool, cpu_threads);
    SetThreadpoolThreadMaximum(tp->pool, cpu_threads);

    tp->cleanup_group = CreateThreadpoolCleanupGroup();

    SetThreadpoolCallbackPool(&tp->callback_environ, tp->pool);
    SetThreadpoolCallbackCleanupGroup(&tp->callback_environ, tp->cleanup_group, NULL);

    return tp;
}

static PTP_WORK thread_pool_add_work_inner(thread_pool* tp, PTP_WORK_CALLBACK function, void* arg_var)
{
    PTP_WORK work = CreateThreadpoolWork(function, arg_var, &tp->callback_environ);
    SubmitThreadpoolWork(work);
    return work;
}

static void thread_pool_destroy_inner(thread_pool* tp)
{
    CloseThreadpoolCleanupGroupMembers(tp->cleanup_group, FALSE, NULL);
    CloseThreadpoolCleanupGroup(tp->cleanup_group);

    DestroyThreadpoolEnvironment(&tp->callback_environ);

    CloseThreadpool(tp->pool);

    free(tp);
}


typedef struct {
    void (*fp)(void*);
    void* args;
} thread_func_and_args;

void CALLBACK thread_caller(PTP_CALLBACK_INSTANCE instance, PVOID arg_var_name, PTP_WORK work) {
    thread_func_and_args* fargs = (thread_func_and_args*)arg_var_name;
    fargs->fp(fargs->args);
}

static thread_pool* tp;
#define MAX_JOB_SLOTS 16
thread_func_and_args job_slots[MAX_JOB_SLOTS];
typedef struct {
    int in_use;
    PTP_WORK work;
} work_handle;
work_handle work_handles[MAX_JOB_SLOTS];

void platform_init_threadpool(int num_threads) {
    for(int i = 0; i < MAX_JOB_SLOTS; i++) {
        work_handles[i].in_use = 0;
    }
    tp = thread_pool_create_inner(num_threads);
}

void platform_add_task(void (*fp)(void* arg), void* arg_ptr) {
    for(int i = 0; i < MAX_JOB_SLOTS; i++) {
        if(work_handles[i].in_use == 0) {
            job_slots[i].args = arg_ptr;
            job_slots[i].fp = fp;
            PTP_WORK work_obj = thread_pool_add_work_inner(tp, thread_caller, &job_slots[i]);
            work_handles[i].in_use = 1;
            work_handles[i].work = work_obj;
            return;
        }
    }
}

void platform_join_threadpool() {
    // wait on all in_use work slots
    for(int i = 0; i < MAX_JOB_SLOTS; i++) {
        if(work_handles[i].in_use) {
            WaitForThreadpoolWorkCallbacks(work_handles[i].work, FALSE);
            CloseThreadpoolWork(work_handles[i].work);
            work_handles[i].in_use = 0;   
        }
    }
}
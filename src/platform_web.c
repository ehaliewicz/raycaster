#include "platform_web.h"
#include "common.h"
#include "my_defs.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
   Canvas blit via JS
   We pass the RGBA pixel pointer straight from WASM memory to the browser.
   emscripten_run_script_int is used for the one call that returns a value
   (focus check); everything else goes through EM_ASM.
   ------------------------------------------------------------------------- */

/* Called once at init: grab a 2D context and size the canvas. */
int g_canvas_w, g_canvas_h;
static void js_init_canvas(int w, int h) {
    g_canvas_w = w;
    g_canvas_h = h;
    EM_ASM({
        var canvas = document.getElementById('canvas');
        canvas.width  = $0;
        canvas.height = $1;
        Module._ctx2d = canvas.getContext('2d');
        /* ImageData reused every frame to avoid per-frame allocation */
        Module._imgData = Module._ctx2d.createImageData($0, $1);
    }, w, h);
}

/* Resize canvas and recreate the ImageData object. */
static void js_resize_canvas(int w, int h) {
    g_canvas_w = w;
    g_canvas_h = h;
    EM_ASM({
        var canvas = document.getElementById('canvas');
        canvas.width  = $0;
        canvas.height = $1;
        Module._imgData = Module._ctx2d.createImageData($0, $1);
    }, w, h);
}

/* Copy WASM memory into the ImageData and push it to the canvas.
   ptr is a byte offset into the WASM heap; num_bytes = w*h*4. */
static void js_blit(const void *ptr, int num_bytes) {
    EM_ASM({
        var src = new Uint8ClampedArray(Module.HEAPU8.buffer, $0, $1);
        Module._imgData.data.set(src);
        Module._ctx2d.putImageData(Module._imgData, 0, 0);
    }, ptr, num_bytes);
}

/* -------------------------------------------------------------------------
   Texture store
   "Textures" on web are plain CPU buffers. We keep 2 as the Win32 code did.
   g_front_tex tracks the last texture updated so end_drawing knows what to blit.
   ------------------------------------------------------------------------- */
#define MAX_TEXTURES 2

typedef struct {
    u32  *pixels;   /* RGBA, width*height*4 bytes */
    int  width;
    int  height;
} cpu_texture;

static cpu_texture   g_textures[MAX_TEXTURES];
static unsigned int  g_tex_ids[MAX_TEXTURES]; /* IDs returned to the caller = 0,1 */
static int           g_front_tex = 0;         /* last updated texture index */
static int           g_tex_w, g_tex_h;        /* shared size (both textures same size) */

void platform_release_textures() {
    for (int i = 0; i < MAX_TEXTURES; i++) {
        if (g_textures[i].pixels) {
            /* use your allocator's free if needed */
            g_textures[i].pixels = NULL;
        }
    }
}

unsigned int *platform_create_textures(int width, int height) {
    g_tex_w = height;
    g_tex_h = width;
    for (int i = 0; i < MAX_TEXTURES; i++) {
        g_textures[i].pixels = my_calloc((size_t)(width * height * 4), "texture");
        g_textures[i].width  = g_tex_w;
        g_textures[i].height = g_tex_h;
        g_tex_ids[i]         = (unsigned int)i;
    }
    return g_tex_ids;
}

u32* first_loaded_image = NULL;
/* Store the caller's RGBA pixels and remember which buffer to blit next frame. */
void platform_update_texture(unsigned int tex, void *pixels, int width, int height) {
    int idx = (int)tex;
    if (idx < 0 || idx >= MAX_TEXTURES) return;
    (void)width; (void)height; // canvas dims are in g_textures from platform_create_textures
    int tex_w = g_textures[idx].width;  // 1280
    int tex_h = g_textures[idx].height; // 1024
    int scale = g_canvas_w/tex_w;
    u32 *src = (u32*)pixels;
    u32 *dst = (u32*)g_textures[idx].pixels;
    for (int y = 0; y < tex_h; y++) {
        for (int x = 0; x < tex_w; x++) {
            for(int y_scale = 0; y_scale < scale; y_scale++) {
                for(int x_scale = 0; x_scale < scale; x_scale++) {
                    dst[(y+y_scale)*scale * g_canvas_w + (g_canvas_w-1-x*scale)+x_scale] = src[x * tex_h + y];
                }
            }
        }
    }
    g_front_tex = idx;
}

/* No-op: the buffer updated via platform_update_texture IS the framebuffer.
   platform_end_drawing() blits it; there is nothing to "draw" here. */
void platform_draw_texture(unsigned int tex, Vector2 pos, float rotation, float scale, int w, int h) {
    (void)tex; (void)pos; (void)rotation; (void)scale; (void)w; (void)h;
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */
static int g_keys[KEY_TABLE_SIZE];
static int g_keys_prev[KEY_TABLE_SIZE];
static int g_last_key_pressed;
static float g_mouse_x, g_mouse_y;
static float g_mouse_dx, g_mouse_dy;
static int g_running = 1;

static int translate_key(const EmscriptenKeyboardEvent *e) {
    const char *k = e->key;

    if (k[0] != '\0' && k[1] == '\0') {
        char c = k[0];
        if (c >= 'a' && c <= 'z') return c - 32;
        if (c >= 'A' && c <= 'Z') return c;
        if (c == ' ')             return KEY_SPACE;
        if (c == '\r' || c == '\n') return KEY_ENTER;
    }

    if (strcmp(k, "Enter")       == 0) return KEY_ENTER;
    if (strcmp(k, "ArrowLeft")   == 0) return KEY_LEFT;
    if (strcmp(k, "ArrowRight")  == 0) return KEY_RIGHT;
    if (strcmp(k, "ArrowUp")     == 0) return KEY_UP;
    if (strcmp(k, "ArrowDown")   == 0) return KEY_DOWN;
    if (strcmp(k, "Shift")       == 0) return KEY_SHIFT;
    if (strcmp(k, "ShiftLeft")   == 0) return KEY_LSHIFT;
    if (strcmp(k, "ShiftRight")  == 0) return KEY_RSHIFT;
    if (strcmp(k, "Control")     == 0) return KEY_CONTROL;
    if (strcmp(k, "ControlLeft") == 0) return KEY_LCONTROL;
    if (strcmp(k, "ControlRight")== 0) return KEY_RCONTROL;

    if (strcmp(k, "0") == 0 && e->location == DOM_KEY_LOCATION_NUMPAD) return KEY_KP_0;
    if (strcmp(k, "9") == 0 && e->location == DOM_KEY_LOCATION_NUMPAD) return KEY_KP_9;

    return -1;
}

static EM_BOOL on_keydown(int type, const EmscriptenKeyboardEvent *e, void *ud) {
    (void)type; (void)ud;
    if (strcmp(e->key, "Escape") == 0) { g_running = 0; return EM_TRUE; }
    int k = translate_key(e);
    
    if (k >= 0 && k < KEY_TABLE_SIZE) {
        g_keys[k] = 1;
        g_last_key_pressed = k;
    }
    return EM_FALSE;
}

static EM_BOOL on_keyup(int type, const EmscriptenKeyboardEvent *e, void *ud) {
    (void)type; (void)ud;
    int k = translate_key(e);
    if (k >= 0 && k < KEY_TABLE_SIZE) g_keys[k] = 0;
    return EM_FALSE;
}

static EM_BOOL on_mousedown(int type, const EmscriptenMouseEvent *e, void *ud) {
    (void)type; (void)ud;
    if (e->button == 0) g_keys[MOUSE_BUTTON_LEFT]   = 1;
    if (e->button == 1) g_keys[MOUSE_BUTTON_MIDDLE] = 1;
    if (e->button == 2) g_keys[MOUSE_BUTTON_RIGHT]  = 1;
    return EM_FALSE;
}

static EM_BOOL on_mouseup(int type, const EmscriptenMouseEvent *e, void *ud) {
    (void)type; (void)ud;
    if (e->button == 0) g_keys[MOUSE_BUTTON_LEFT]   = 0;
    if (e->button == 1) g_keys[MOUSE_BUTTON_MIDDLE] = 0;
    if (e->button == 2) g_keys[MOUSE_BUTTON_RIGHT]  = 0;
    return EM_FALSE;
}

static EM_BOOL on_mousemove(int type, const EmscriptenMouseEvent *e, void *ud) {
    (void)type; (void)ud;
    g_mouse_x   = (float)e->targetX;
    g_mouse_y   = (float)e->targetY;
    g_mouse_dx += (float)e->movementX;
    g_mouse_dy += (float)e->movementY;
    return EM_FALSE;
}

int platform_is_key_down(int key) {
    if (key < 0 || key >= KEY_TABLE_SIZE) return 0;
    return g_keys[key];
}

int platform_is_key_pressed(int key) {
    if (key < 0 || key >= KEY_TABLE_SIZE) return 0;
    return g_keys[key] && !g_keys_prev[key];
}

int platform_get_key_pressed() {
    int r = g_last_key_pressed;
    g_last_key_pressed = 0;
    return r;
}

int platform_is_mouse_button_pressed(int button) {
    return platform_is_key_pressed(button);
}

Vector2 platform_get_mouse_delta() {
    return (Vector2){ .x = g_mouse_dx * 0.5f , .y = g_mouse_dy * 0.5f  };
}

Vector2 platform_get_mouse_position() {
    return (Vector2){ .x = g_mouse_x, .y = g_mouse_y };
}

void platform_set_mouse_position(int x, int y) { (void)x; (void)y; }

void platform_show_cursor() { 
    EM_ASM({
        document.getElementById('canvas').style.cursor = 'default';
    });
    emscripten_exit_pointerlock(); 
}
void platform_hide_cursor() {
    EM_ASM({
        document.getElementById('canvas').style.cursor = 'none';
    });
     emscripten_request_pointerlock("#canvas", 1); 
}

/* -------------------------------------------------------------------------
   Timing
   ------------------------------------------------------------------------- */
static float g_time_start;
static float g_frame_start;
static float g_frame_end;

void platform_init_time(void) {
    g_time_start = (float)emscripten_get_now() * 0.001f;
}

float platform_get_time() {
    return (float)emscripten_get_now() * 0.001f - g_time_start;
}

float platform_get_frame_time() {
    return g_frame_end - g_frame_start;
}

/* -------------------------------------------------------------------------
   Window
   ------------------------------------------------------------------------- */
static int g_screen_w, g_screen_h;

void platform_init_window(int width, int height, const char *title) {
    (void)title;
    g_screen_w = width;
    g_screen_h = height;

    js_init_canvas(width, height);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, on_keydown);
    emscripten_set_keyup_callback  (EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, on_keyup);
    emscripten_set_mousedown_callback("#canvas", NULL, 1, on_mousedown);
    emscripten_set_mouseup_callback  ("#canvas", NULL, 1, on_mouseup);
    emscripten_set_mousemove_callback("#canvas", NULL, 1, on_mousemove);

    platform_init_time();
}

void platform_set_window_size(int width, int height) {
    g_screen_w = width;
    g_screen_h = height;
    js_resize_canvas(width, height);
}

void platform_set_vsync(int enabled)  { (void)enabled; /* browser vsync always on */ }
void platform_set_windowed()          { /* no-op */ }
void platform_set_fullscreen() {
    return;
    EmscriptenFullscreenStrategy s = {0};
    s.scaleMode                 = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
    s.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF;
    s.filteringMode             = EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST;
    emscripten_request_fullscreen_strategy("#canvas", 1, &s);
}

int platform_is_window_focused() { return 1; }

void platform_begin_frame() {
}

void platform_end_frame() {   
    memcpy(g_keys_prev, g_keys, sizeof(g_keys));
    g_mouse_dx = 0;
    g_mouse_dy = 0;
}

/* Ticks prev-key state and clears mouse delta, exactly like the Win32 version. */
int platform_window_should_close() {
    return !g_running;
}

/* -------------------------------------------------------------------------
   Drawing
   begin_drawing / end_drawing bracket your frame exactly as before.
   end_drawing blits the last-updated texture buffer to the canvas.
   ------------------------------------------------------------------------- */
void platform_begin_drawing() {
    g_frame_start = platform_get_time();
}

void platform_end_drawing() {
    cpu_texture *t = &g_textures[g_front_tex];
    if (t->pixels) {
        js_blit(t->pixels, t->width * t->height * 4);
    }
    g_frame_end = platform_get_time();
}

void platform_draw_text(const char *text, Vector2 position, float fontSize, float spacing, u32 tint) {
    (void)text; (void)position; (void)fontSize; (void)spacing; (void)tint;
}

/* -------------------------------------------------------------------------
   File I/O — uses Emscripten's virtual FS (assets bundled via --preload-file)
   ------------------------------------------------------------------------- */
int platform_save_file_data(const char *path, void *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    return (int)written;
}

u8 *platform_load_file_data(const char *path, int *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        debug_printf("error loading file %s\n", path);
        *out_size = 0;
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    u8 *data = my_malloc((size_t)size, "file load");
    fread(data, 1, (size_t)size, f);
    fclose(f);
    *out_size = (int)size;
    return data;
}


/* Reads a TGA file and returns RGBA pixels, handling the vertical-flip bit. */
u8 *platform_load_image(const char *path, int expected_width, int expected_height) {
    int size;
    u8 *data = platform_load_file_data(path, &size);
    if (!data) { debug_printf("error loading image %s\n", path); return NULL; }

    int w    = data[12] | (data[13] << 8);
    int h    = data[14] | (data[15] << 8);
    int bpp  = data[16] / 8;
    int flip = !(data[17] & 0x20);

    if (w != expected_width)  { debug_printf("Expected width %i got %i\n",  expected_width,  w); return NULL; }
    if (h != expected_height) { debug_printf("Expected height %i got %i\n", expected_height, h); return NULL; }

    u8 *src = data + 18;
    u8 *out = my_malloc((size_t)(w * h * 4), "image rgba");

    if(first_loaded_image == NULL) {
        first_loaded_image = (u32*)out;
    }
    for (int y = 0; y < h; y++) {
        int src_row = flip ? (h - 1 - y) : y;
        for (int x = 0; x < w; x++) {
            u8 *s = src + (src_row * w + x) * bpp;
            u8 *d = out + (y * w + x) * 4;
            d[0] = s[2]; /* R (TGA is BGR on disk) */
            d[1] = s[1]; /* G */
            d[2] = s[0]; /* B */
            d[3] = s[3]; /* A */
        }
    }
    return out;
}

void platform_unload_image(u8 *img_data) { (void)img_data; }

/* -------------------------------------------------------------------------
   Thread pool stubs — all tasks run synchronously
   ------------------------------------------------------------------------- */
jobpool *platform_init_threadpool(int num_threads) {
    (void)num_threads;
    return my_calloc(sizeof(jobpool), "job pool");
}

void platform_add_task(jobpool *jp, void (*fp)(void *arg), void *arg_ptr) {
    for (int i = 0; i < MAX_JOB_SLOTS; i++) {
        if (!jp->work_handles[i].in_use) {
            jp->job_slots[i].fp   = fp;
            jp->job_slots[i].args = arg_ptr;
            jp->work_handles[i].in_use = 1;
            fp(arg_ptr);                   /* run immediately */
            jp->work_handles[i].in_use = 0;
            return;
        }
    }
    debug_printf("platform_add_task: no free slots\n");
}

void platform_join_threadpool(jobpool *jp) { (void)jp; /* already done */ }
#include "assert.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>

#include "common.h"
#include "my_defs.h"
#include "platform.h"

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

void platform_begin_frame() {
    my_memcpy(g_keys_prev, g_keys, sizeof(int)*256);
    g_mouse_dx = 0; g_mouse_dy = 0;
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black
    glClear(GL_COLOR_BUFFER_BIT);
}

void platform_end_frame() {

}


int platform_window_should_close() {
    if(g_keys[VK_ESCAPE]) { g_running = 0; }
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

typedef ptrdiff_t GLsizeiptr;

typedef long long int (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef void   (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const char**, const GLint*);
typedef void   (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void   (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void   (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void   (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);

// Uniforms
typedef GLint  (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const char*);
typedef void   (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void   (APIENTRY *PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void   (APIENTRY *PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void   (APIENTRY *PFNGLUNIFORM4FPROC)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);

// VBO/VAO
typedef void   (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void   (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void   (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void   (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void   (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void   (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void   (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);

typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, char*);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, char*);

typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);

typedef void  (*PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint *framebuffers);
typedef void  (*PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);

typedef void (*PFNGLGENRENDERBUFFERSPROC)      (GLsizei n, GLuint *renderbuffers);
typedef void (*PFNGLBINDRENDERBUFFERPROC)       (GLenum target, GLuint renderbuffer);
typedef void (*PFNGLRENDERBUFFERSTORAGEPROC)    (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (*PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);

//static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

#define GL_ROUTINES                                     \
    X(PFNGLCREATESHADERPROC,             glCreateShader)      \
    X(PFNWGLSWAPINTERVALEXTPROC,         wglSwapIntervalEXT)  \
    X(PFNGLSHADERSOURCEPROC,             glShaderSource)        \
    X(PFNGLCOMPILESHADERPROC,            glCompileShader)      \
    X(PFNGLCREATEPROGRAMPROC,            glCreateProgram)      \
    X(PFNGLATTACHSHADERPROC,             glAttachShader)      \
    X(PFNGLLINKPROGRAMPROC,              glLinkProgram)      \
    X(PFNGLUSEPROGRAMPROC,               glUseProgram)      \
    X(PFNGLGETUNIFORMLOCATIONPROC,       glGetUniformLocation)      \
    X(PFNGLUNIFORM1IPROC,               glUniform1i)      \
    X(PFNGLUNIFORM1FPROC,               glUniform1f)      \
    X(PFNGLUNIFORM2FPROC,               glUniform2f)      \
    X(PFNGLUNIFORM4FPROC,               glUniform4f)      \
    X(PFNGLGENBUFFERSPROC,              glGenBuffers)      \
    X(PFNGLBINDBUFFERPROC,              glBindBuffer)      \
    X(PFNGLBUFFERDATAPROC,              glBufferData)      \
    X(PFNGLGENVERTEXARRAYSPROC,         glGenVertexArrays)      \
    X(PFNGLBINDVERTEXARRAYPROC,         glBindVertexArray)      \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)      \
    X(PFNGLVERTEXATTRIBPOINTERPROC,     glVertexAttribPointer)  \
    X(PFNGLGETSHADERIVPROC,       glGetShaderiv)           \
    X(PFNGLGETSHADERINFOLOGPROC,  glGetShaderInfoLog)          \
    X(PFNGLGETPROGRAMIVPROC,      glGetProgramiv)              \
    X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)           \
    X(PFNGLACTIVETEXTUREPROC, glActiveTexture)                  \
    X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers)               \
    X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer)              \
    X(PFNGLGENRENDERBUFFERSPROC,       glGenRenderbuffers)       \
    X(PFNGLBINDRENDERBUFFERPROC,       glBindRenderbuffer)             \
    X(PFNGLRENDERBUFFERSTORAGEPROC,    glRenderbufferStorage)          \
    X(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer)



#define X(type, name) static type name;
GL_ROUTINES
#undef X 

#define GL_VERTEX_SHADER   0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_ARRAY_BUFFER    0x8892
//#define GL_STATIC_DRAW     0x88B4
//#define GL_DYNAMIC_DRAW     0x88B8
#define GL_FRAMEBUFFER 0x8D40

#define GL_STATIC_DRAW          0x88E4
#define GL_DYNAMIC_DRAW         0x88E8

#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82

#define WGL_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB       0x2092
#define WGL_CONTEXT_FLAGS_ARB               0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB        0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB    0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_DEBUG_BIT_ARB           0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002

#define GL_COLOR_ATTACHMENT0  0x8CE0
#define GL_RENDERBUFFER       0x8D41
#define GL_RGBA8              0x8058
#define GL_TEXTURE0 0x84C0

GLuint compile_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint status;
    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        printf("shader error: %s\n", log);
    }
    return s;
}

GLuint vert;
GLuint seg01_frag, seg23_frag, all_segs_frag;
GLuint seg01_prog, seg23_prog, all_segs_prog;
GLuint vbo;
GLuint vao;

const char* vert_shader_src = "#version 330 core\n"
"layout (location = 0) in vec3 vertexPos;\n"
"layout (location = 1) in vec4 vertexTexCoord;\n"
"out vec3 vertex;\n"
"out vec4 uv;\n"
"void main()\n"
"{\n"
"    vertex = vertexPos;\n"
"    uv = vertexTexCoord;\n"
"    gl_Position = vec4(vertexPos, 1.0);\n"
"}\n";


const char* seg01_frag_shader_src = "#version 330 core\n"
"in vec3 vertex;\n"
"in vec4 uv;\n"
"out vec4 color;\n"
"uniform sampler2D rayBuffer;\n"
"uniform vec4 rayScales, rayOffsets;\n"
"void main() {\n"
"    float y = 1-(vertex.y + 1.0) * 0.5;\n"
"    float x = clamp(uv.x, 0.0, 1.0) / (uv.x + uv.y);\n"
"    int tri_idx = int(uv.w);\n"
"    float ray_buffer_scale = rayScales[tri_idx];\n"
"    float ray_buffer_offset = rayOffsets[tri_idx];\n"
"    x = ray_buffer_offset + x * ray_buffer_scale;\n"
"    x = clamp(x, ray_buffer_offset + 0.001, ray_buffer_offset + ray_buffer_scale - 0.001);\n"
"    vec4 sample = texture2D(rayBuffer, vec2(y,x));\n"
"    color = sample;\n"
"}\n";

const char* seg23_frag_shader_src = "#version 330 core\n"
"in vec3 vertex;\n"
"in vec4 uv;\n"
"out vec4 color;\n"
"uniform sampler2D rayBuffer;\n"
"uniform vec4 rayScales, rayOffsets;\n"
"void main() {\n"
"    float y = 1-(vertex.x + 1.0) * 0.5;\n" // only difference between these two, we use the screen x coord here
"    float x = clamp(uv.x, 0.0, 1.0) / (uv.x + uv.y);\n"
"    int tri_idx = int(uv.w)\n;"
"    float ray_buffer_scale = rayScales[tri_idx];\n"
"    float ray_buffer_offset = rayOffsets[tri_idx];\n"
"    x = ray_buffer_offset + x * ray_buffer_scale;\n"
"    x = clamp(x, ray_buffer_offset + 0.001, ray_buffer_offset + ray_buffer_scale - 0.001);\n"
"    vec4 sample = texture2D(rayBuffer, vec2(y,x));\n"
"    color = sample;\n"
"}\n";

/*
const char* all_segs_frag_shader_src = "#version 330 core\n"
"in vec3 vertex;\n"
"in vec4 uv;\n"
"out vec4 color;\n"
"uniform vec2 screen_vp;\n"
"uniform float slope;\n"
"uniform vec2 iResolution;\n"
"uniform sampler2D rayBuffer01, rayBuffer23;\n"
"uniform vec4 rayScales, rayOffsets;\n"
"uniform vec2 seg0_v1, seg0_v2;\n"
"uniform vec2 seg1_v1, seg1_v2;\n"
"uniform vec2 seg2_v1, seg2_v2;\n"
"uniform vec2 seg3_v1, seg3_v2;\n"
"void main() {\n"
"    float seg01_tl_dr_slope = slope;\n"
"    float seg01_tr_dl_slope = -seg01_tl_dr_slope;\n"
"    vec2 d_vp = gl_FragCoord.xy - screen_vp;\n"
"    float cur_slope = (d_vp.y/d_vp.x);\n"
"    bool in_seg01 = (cur_slope < seg01_tl_dr_slope || cur_slope > -seg01_tl_dr_slope);\n"
"    bool in_seg1 = (in_seg01 && gl_FragCoord.y < screen_vp.y);\n"
"    bool in_seg3 = (!in_seg01) && (gl_FragCoord.x < screen_vp.x);\n"
"    int seg_idx = (in_seg01 ? (in_seg1 ? 1 : 0) : (in_seg3 ? 3 : 2));\n"
"    float ray_buffer_scale = rayScales[seg_idx];\n"
"    float ray_buffer_offset = (seg_idx == 1 || seg_idx == 3) ? rayOffsets[seg_idx] : 0.;\n"
"    float u;\n"
"    float v;\n"
"    float tr_dl_x = screen_vp.x + d_vp.y * 1.0/seg01_tr_dl_slope; // the x position of the edge at this y\n"
"    float tl_dr_x = screen_vp.x + d_vp.y * 1.0/seg01_tl_dr_slope; // the x position of the edge at this y\n"
"    float tr_dl_y = screen_vp.y + d_vp.x * seg01_tr_dl_slope;    // the y position of the edge at this x\n"
"    float tl_dr_y = screen_vp.y + d_vp.x * seg01_tl_dr_slope;    // tde y position of the edge at this x\n"
//"    float tot_dx = abs(2.*(tr_dl_x-screen_vp.x));\n"
//"    float tot_dy = abs(2.*(tl_dr_y-screen_vp.y));\n"
"    float tot_dx = abs(tr_dl_x - tl_dr_x);\n"
"    float tot_dy = abs(tr_dl_y - tl_dr_y);\n"
"    if(in_seg01) {\n"
"        v = gl_FragCoord.y/iResolution.y;\n"
"        u = clamp((gl_FragCoord.x-(in_seg1 ? tr_dl_x : tl_dr_x))/tot_dx, 0., 1.);\n"
"        u = ray_buffer_offset + u * ray_buffer_scale;\n"
"        color = texture2D(rayBuffer01, vec2(1.0-v,u));\n"
"        //color =  vec4(1.0,1.0,1.0,1.0);\n"
"    } else {\n"
"        v = gl_FragCoord.x/iResolution.x;\n"
"        u = clamp((gl_FragCoord.y-(in_seg3 ? tr_dl_y : tl_dr_y))/tot_dy, 0., 1.);\n"
//"        if(screen_vp.y > iResolution.y) { u *= 2.; }\n"
"        if(screen_vp.y < 0.) { u /= 2.; }\n"
"        u = ray_buffer_offset + u * ray_buffer_scale;\n"
"        color = texture2D(rayBuffer23, vec2(1.0-v,u));\n"
"    }\n"
"    color = vec4(1.0-v,u,0.0,1.0);\n"
"    if(abs(d_vp.x) < 1.0 && abs(d_vp.y) < 1.0) {\n"
"        color = vec4(1.0,0.0,0.0,1.0);\n"
"    }\n"
"}\n";
*/

//Here's the full fragment shader:
//c
const char* all_segs_frag_shader_src = "#version 330 core\n"
"in vec3 vertex;\n"
"in vec4 uv;\n"
"out vec4 color;\n"
"uniform vec2 screen_vp;\n"
"uniform float slope;\n"
"uniform vec2 iResolution;\n"
"uniform sampler2D rayBuffer01, rayBuffer23;\n"
"uniform vec4 rayScales, rayOffsets;\n"
"uniform vec2 seg0_v1, seg0_v2;\n"
"uniform vec2 seg1_v1, seg1_v2;\n"
"uniform vec2 seg2_v1, seg2_v2;\n"
"uniform vec2 seg3_v1, seg3_v2;\n"
"void main() {\n"
"    float seg01_tl_dr_slope = slope;\n"
"    float seg01_tr_dl_slope = -seg01_tl_dr_slope;\n"
"    vec2 d_vp = gl_FragCoord.xy - screen_vp;\n"
"    float frag_slope = d_vp.y / d_vp.x;\n"
"    bool in_seg01 = (frag_slope < seg01_tl_dr_slope || frag_slope > -seg01_tl_dr_slope);\n"
"    bool in_seg1 = (in_seg01 && gl_FragCoord.y < screen_vp.y);\n"
"    bool in_seg3 = (!in_seg01) && (gl_FragCoord.x < screen_vp.x);\n"
"    int seg_idx = (in_seg01 ? (in_seg1 ? 1 : 0) : (in_seg3 ? 3 : 2));\n"
"    float ray_buffer_scale = rayScales[seg_idx];\n"
"    float ray_buffer_offset = rayOffsets[seg_idx];\n"
"    vec2 p = gl_FragCoord.xy;\n"
"    vec2 p0 = screen_vp;\n"
"    vec2 p1, p2;\n"
"    if(seg_idx == 0) { p1 = seg0_v1; p2 = seg0_v2; }\n"
"    else if(seg_idx == 1) { p1 = seg1_v1; p2 = seg1_v2; }\n"
"    else if(seg_idx == 2) { p1 = seg2_v1; p2 = seg2_v2; }\n"
"    else                  { p1 = seg3_v1; p2 = seg3_v2; }\n"
"    float denom = (p1.y-p2.y)*(p0.x-p2.x) + (p2.x-p1.x)*(p0.y-p2.y);\n"
"    float w1 = ((p2.y-p0.y)*(p.x-p2.x) + (p0.x-p2.x)*(p.y-p2.y)) / denom;\n"
"    float w2 = 1.0 - (((p1.y-p2.y)*(p.x-p2.x) + (p2.x-p1.x)*(p.y-p2.y)) / denom) - w1;\n"
"    float u = clamp(w1, 0.0, 1.0) / (w1 + w2);\n"
"    u = ray_buffer_offset + u * ray_buffer_scale;\n"
"    u = clamp(u, ray_buffer_offset + 0.001, ray_buffer_offset + ray_buffer_scale - 0.001);\n"
"    float v;\n"
"    if(in_seg01) {\n"
"        v = gl_FragCoord.y / iResolution.y;\n"
"        color = texture2D(rayBuffer01, vec2(1.0-v, u));\n"
"    } else {\n"
"        v = gl_FragCoord.x / iResolution.x;\n"
"        color = texture2D(rayBuffer23, vec2(1.0-v, u));\n"
"    }\n"
//"    color = vec4(1.0-v, u, 0.0, 1.0);\n"
//"    if(abs(d_vp.x) < 1.0 && abs(d_vp.y) < 1.0) {\n"
//"        color = vec4(1.0, 0.0, 0.0, 1.0);\n"
//"    }\n"
"}\n";



unsigned int edit_fbo = 0;
unsigned int edit_rbo = 0;

void platform_bind_edit_framebuffer() {
    if(edit_fbo == 0) {
        glGenFramebuffers(1, &edit_fbo);
        glGenRenderbuffers(1, &edit_rbo);
        
        glBindFramebuffer(GL_FRAMEBUFFER, edit_fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, edit_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, OUTPUT_WIDTH, OUTPUT_HEIGHT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, edit_rbo);
    
    }

    glBindFramebuffer(GL_FRAMEBUFFER, edit_fbo);
}

u32 platform_read_pixel_at(int x, int y) {
    u32 pix;
    glReadPixels(x, y, 1, 1,  GL_RGB, GL_UNSIGNED_BYTE, &pix);
    return pix;
}
void platform_unbind_edit_framebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);

void platform_init_window(int width, int height, const char *title) {
    
    SetProcessDPIAware();

    if(g_keys == NULL) {
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

    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
    (PFNWGLCREATECONTEXTATTRIBSARBPROC)(void*)wglGetProcAddress("wglCreateContextAttribsARB");
    
    //wglMakeCurrent(NULL, NULL);
    //wglDeleteContext(g_rc);


    //int attribs[] = {
    //    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    //    WGL_CONTEXT_MINOR_VERSION_ARB, 2,
    //    WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    //    0 // terminate
    //};
    //g_rc = wglCreateContextAttribsARB(g_dc, NULL, attribs);
    //wglMakeCurrent(g_dc, g_rc);

#define X(type,name) name = (type)(void*)wglGetProcAddress(#name);
        GL_ROUTINES;
#undef X


    update_viewport(width, height);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    vert = compile_shader(GL_VERTEX_SHADER, vert_shader_src);
    seg01_frag = compile_shader(GL_FRAGMENT_SHADER, seg01_frag_shader_src);

    seg01_prog = glCreateProgram();
    glAttachShader(seg01_prog, vert);
    glAttachShader(seg01_prog, seg01_frag);
    glLinkProgram(seg01_prog);
    GLint status;
    glGetProgramiv(seg01_prog, GL_LINK_STATUS, &status);
    if (!status) {
        char log[512];
        glGetProgramInfoLog(seg01_prog, 512, NULL, log);
        printf("link error: %s\n", log);
        exit(1);
    }

    seg23_frag = compile_shader(GL_FRAGMENT_SHADER, seg23_frag_shader_src);
    seg23_prog = glCreateProgram();
    glAttachShader(seg23_prog, vert);
    glAttachShader(seg23_prog, seg23_frag);
    glLinkProgram(seg23_prog);
    glGetProgramiv(seg23_prog, GL_LINK_STATUS, &status);
    if (!status) {
        char log[512];
        glGetProgramInfoLog(seg23_prog, 512, NULL, log);
        printf("link error: %s\n", log);
        exit(1);
    }

    
    all_segs_frag = compile_shader(GL_FRAGMENT_SHADER, all_segs_frag_shader_src);
    all_segs_prog = glCreateProgram();
    glAttachShader(all_segs_prog, vert);
    glAttachShader(all_segs_prog, all_segs_frag);
    glLinkProgram(all_segs_prog);
    glGetProgramiv(all_segs_prog, GL_LINK_STATUS, &status);
    if (!status) {
        char log[512];
        glGetProgramInfoLog(all_segs_prog, 512, NULL, log);
        printf("link error: %s\n", log);
        exit(1);
    }
    

    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);



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


void platform_release_texture(int tex) { 
    unsigned int gpu_tex[1] = { tex };
    glDeleteTextures(1, gpu_tex);
}


unsigned int platform_create_texture(int width, int height) {
    unsigned int gpu_textures[1];
    glGenTextures(1, gpu_textures);
    glBindTexture(GL_TEXTURE_2D, gpu_textures[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
    return gpu_textures[0];
}

void platform_update_texture(unsigned int tex, void *pixels, int xoff, int yoff, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, xoff, yoff, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB,  GL_UNSIGNED_SHORT_5_6_5, pixels);
}


void platform_draw_texture(unsigned int tex, Vector2 pos, float rotation, float scale, int w, int h) {
    glUseProgram(0);
    //glBindTexture(GL_TEXTURE_2D, tex);
    
    glActiveTexture(GL_TEXTURE0+0); // texture unit 0
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


void platform_draw_full_quad(
    int seg01_tex_handle, int seg23_tex_handle,
    float screen_vp_x, float screen_vp_y,
    float top_left_endpoint_x, float top_left_endpoint_y,
    float bot_right_endpoint_x, float bot_right_endpoint_y,
    float seg0_v1[2], float seg0_v2[2],
    float seg1_v1[2], float seg1_v2[2],
    float seg2_v1[2], float seg2_v2[2],
    float seg3_v1[2], float seg3_v2[2],
    float offsets[4],
    float scales[4]
) {

    float scale_output = OUTPUT_WIDTH/RENDER_WIDTH;

    glUseProgram(all_segs_prog);
    glUniform1i(glGetUniformLocation(all_segs_prog, "rayBuffer01"), 0);
    glUniform1i(glGetUniformLocation(all_segs_prog, "rayBuffer23"), 1);
    glUniform4f(glGetUniformLocation(all_segs_prog, "rayOffsets"), offsets[0], offsets[1], offsets[2], offsets[3]);
    glUniform4f(glGetUniformLocation(all_segs_prog, "rayScales"), scales[0], scales[1], scales[2], scales[3]);

    glUniform2f(glGetUniformLocation(all_segs_prog, "seg0_v1"), seg0_v1[0]*scale_output, seg0_v1[1]*scale_output);
    glUniform2f(glGetUniformLocation(all_segs_prog, "seg0_v2"), seg0_v2[0]*scale_output, seg0_v2[1]*scale_output);
    glUniform2f(glGetUniformLocation(all_segs_prog, "seg1_v1"), seg1_v1[0]*scale_output, seg1_v1[1]*scale_output);
    glUniform2f(glGetUniformLocation(all_segs_prog, "seg1_v2"), seg1_v2[0]*scale_output, seg1_v2[1]*scale_output);
    glUniform2f(glGetUniformLocation(all_segs_prog, "seg2_v1"), seg2_v1[0]*scale_output, seg2_v1[1]*scale_output);
    glUniform2f(glGetUniformLocation(all_segs_prog, "seg2_v2"), seg2_v2[0]*scale_output, seg2_v2[1]*scale_output);
    glUniform2f(glGetUniformLocation(all_segs_prog, "seg3_v1"), seg3_v1[0]*scale_output, seg3_v1[1]*scale_output);
    glUniform2f(glGetUniformLocation(all_segs_prog, "seg3_v2"), seg3_v2[0]*scale_output, seg3_v2[1]*scale_output);


    glUniform2f(glGetUniformLocation(all_segs_prog, "screen_vp"), screen_vp_x*scale_output, screen_vp_y*scale_output);
    float slope;
    if(bot_right_endpoint_x == 0) {
        slope = (top_left_endpoint_y-screen_vp_y)/(top_left_endpoint_x-screen_vp_x);
    } else {
        slope = (bot_right_endpoint_y-screen_vp_y)/(bot_right_endpoint_x-screen_vp_x);
    }
    glUniform1f(glGetUniformLocation(all_segs_prog, "slope"), slope);
    
    glUniform2f(glGetUniformLocation(all_segs_prog, "iResolution"), ((float)OUTPUT_WIDTH), ((float)OUTPUT_HEIGHT));

    float attributes[3*3] = {
        -1.0f*scale_output, -1.0f*scale_output, 0.5f,
        3.0f*scale_output,  -1.0f*scale_output, 0.5f,
        -1.0f*scale_output,  3.0f*scale_output, 0.5f,
    };
    glActiveTexture(GL_TEXTURE0+0); // texture unit 0
    glBindTexture(GL_TEXTURE_2D, seg01_tex_handle);
    glActiveTexture(GL_TEXTURE0+1); // texture unit 0
    glBindTexture(GL_TEXTURE_2D, seg23_tex_handle);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(3*3), attributes, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 3);

}


void platform_draw_segments(
    int num_segments,
    unsigned int tex,
    int seg_idx,
    float attributes[],
    float offsets[4],
    float scales[4])
{
    GLuint prog = (seg_idx < 2) ? seg01_prog : seg23_prog;
    glBindTexture(GL_TEXTURE_2D, tex);
    glUseProgram(prog);
    glUniform4f(glGetUniformLocation(prog, "rayScales"), scales[0], scales[1], scales[2], scales[3]);
    glUniform4f(glGetUniformLocation(prog, "rayOffsets"), offsets[0], offsets[1], offsets[2], offsets[3]);


    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(7*3*num_segments), attributes, GL_DYNAMIC_DRAW); // 7*3*num_segments, 1 = 21, 2 = 42
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLES, 0, 3*num_segments);

    return;
}



float start_time, end_time;
void platform_begin_drawing() {
    start_time = platform_get_time();
}

void platform_end_drawing() {
    SwapBuffers(g_dc);
    end_time = platform_get_time();
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
    u8* data = my_malloc(size, "file load");

    //u8 *data = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
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
    thread_pool* tp = (thread_pool*)my_calloc(sizeof(thread_pool), "thread pool");

    InitializeThreadpoolEnvironment(&tp->callback_environ);

    tp->pool = CreateThreadpool(NULL);
    printf("CREATING THREADPOOL %i threads\n", cpu_threads);
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




void CALLBACK thread_caller(PTP_CALLBACK_INSTANCE instance, PVOID arg_var_name, PTP_WORK work) {
    thread_func_and_args* fargs = (thread_func_and_args*)arg_var_name;
    fargs->fp(fargs->args);
}



jobpool* platform_init_threadpool(int num_threads) {
    thread_pool* tp = thread_pool_create_inner(num_threads);
    jobpool* jp = my_malloc(sizeof(jobpool), "job pool");
    for(int i = 0; i < MAX_JOB_SLOTS; i++) {
        jp->work_handles[i].in_use = 0;
    }
    jp->threadpool = tp;
    return jp;
}

void platform_add_task(jobpool* jp, void (*fp)(void* arg), void* arg_ptr) {
    thread_pool* tp = (thread_pool*)(jp->threadpool);

    for(int i = 0; i < MAX_JOB_SLOTS; i++) {
        if(jp->work_handles[i].in_use == 0) {
            jp->job_slots[i].args = arg_ptr;
            jp->job_slots[i].fp = fp;

            PTP_WORK work_obj = thread_pool_add_work_inner(tp, thread_caller, jp->job_slots+i);
            jp->work_handles[i].in_use = 1;
            jp->work_handles[i].work = work_obj;
            return;
        }
    }
}

void platform_join_threadpool(jobpool* jp) {
    thread_pool* tp = (thread_pool*)jp->threadpool;
    // wait on all in_use work slots
    for(int i = 0; i < MAX_JOB_SLOTS; i++) {
        if(jp->work_handles[i].in_use) {
            WaitForThreadpoolWorkCallbacks((PTP_WORK)(jp->work_handles[i].work), FALSE);
            CloseThreadpoolWork((PTP_WORK)(jp->work_handles[i].work));
            jp->work_handles[i].in_use = 0;   
        }
    }
}


void platform_play_sound(const char* sound_path) {
    PlaySound(sound_path, NULL, SND_FILENAME|SND_ASYNC);
}
/*
 * Windows screen saver entry point.  Replaces main.c for the .scr build,
 * mirroring the role SoFICubeView.m plays on macOS: this file owns the
 * window, the GL context, and the frame loop, while mesh.c, render.c,
 * logo.c, and font.c are shared unchanged.
 *
 * SCREENSAVER_BUILD comes from the build flags, not a #define here: font.c
 * and render.c test it too, and all of them must agree.
 */
#include "cube.h"

#include <ctype.h>
#include <windowsx.h>

/* ---- globals normally provided by main.c ---- */

struct WinState win = {800, 600};
int (*cube_get_elapsed_ms)(void) = NULL;

/* ---- timing ---- */

static ULONGLONG start_ticks;
static LARGE_INTEGER qpc_freq;

static int tick_elapsed_ms(void)
{
    return (int)(GetTickCount64() - start_ticks);
}

/* ---- state ---- */

static HDC   hdc;
static HGLRC hglrc;
static int   is_preview;
static int   gl_ready;

static struct {
    float pitch, yaw;
} rotation = {25.0f, 0.0f};

/* ---- GL init (mirrors main.c init_gl) ---- */

static void setup_gl(void)
{
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat light_pos[]      = {2.0f, 3.0f, 4.0f, 0.0f};
    GLfloat light_ambient[]  = {0.3f, 0.3f, 0.3f, 1.0f};
    GLfloat light_diffuse[]  = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat mat_shininess[]  = {50.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
}

/*
 * Windows launches a .scr from System32, so the relative asset paths in
 * cube.h do not resolve.  Look next to the executable instead: the
 * build-tree layout first, then flat beside an installed .scr.
 */
static const char *asset_path(const char *name)
{
    /* Room for the directory plus the longest subpath appended below. */
    static char buf[MAX_PATH * 2];
    char dir[MAX_PATH];

    DWORD len = GetModuleFileNameA(NULL, dir, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return name;

    char *slash = strrchr(dir, '\\');
    if (slash) *slash = '\0';

    int n = snprintf(buf, sizeof(buf), "%s\\screensaver\\Resources\\%s", dir, name);
    if (n > 0 && (size_t)n < sizeof(buf) &&
        GetFileAttributesA(buf) != INVALID_FILE_ATTRIBUTES)
        return buf;

    n = snprintf(buf, sizeof(buf), "%s\\%s", dir, name);
    if (n > 0 && (size_t)n < sizeof(buf)) return buf;

    return name;
}

/*
 * larsseit.ttf is not distributable, and this build has no GLUT bitmap-font
 * fallback, so a missing font means no stats overlay at all.  Fall back to a
 * stock Windows face to keep it visible.
 */
static const char *font_path(void)
{
    static char buf[MAX_PATH * 2];
    static const char *stock[] = {"segoeui.ttf", "arial.ttf", "tahoma.ttf"};

    const char *p = asset_path("larsseit.ttf");
    if (GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES) return p;

    char windir[MAX_PATH];
    UINT len = GetWindowsDirectoryA(windir, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return p;

    for (size_t i = 0; i < COUNTOF(stock); i++) {
        int n = snprintf(buf, sizeof(buf), "%s\\Fonts\\%s", windir, stock[i]);
        if (n > 0 && (size_t)n < sizeof(buf) &&
            GetFileAttributesA(buf) != INVALID_FILE_ATTRIBUTES)
            return buf;
    }

    return p;
}

static void resize(int w, int h)
{
    if (h == 0) h = 1;
    win.w = w;
    win.h = h;

    if (!gl_ready) return;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, (double)w / (double)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

static void render_frame(void)
{
    rotation.yaw   += SPIN_YAW;
    rotation.pitch += SPIN_PITCH;
    if (rotation.yaw   > 360.0f) rotation.yaw   -= 360.0f;
    if (rotation.pitch > 360.0f) rotation.pitch -= 360.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, CAMERA_Z);
    glRotatef(rotation.pitch, 1.0f, 0.0f, 1.0f);
    glRotatef(rotation.yaw,   0.0f, 1.0f, 1.0f);

    draw_cube();

    if (!is_preview) {
        update_stats();
        draw_stats_overlay();
    }

    SwapBuffers(hdc);
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    /* Windows delivers a spurious move when the saver starts; require real travel. */
    static POINT first_pos = {-1, -1};

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        resize(LOWORD(lp), HIWORD(lp));
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_SETCURSOR:
        if (!is_preview) {
            SetCursor(NULL);
            return TRUE;
        }
        break;

    case WM_MOUSEMOVE:
        if (!is_preview) {
            POINT p = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            if (first_pos.x < 0) {
                first_pos = p;
            } else if (abs(p.x - first_pos.x) > 8 || abs(p.y - first_pos.y) > 8) {
                PostQuitMessage(0);
            }
        }
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (!is_preview) PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

static int init_gl_context(HWND hwnd)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    hdc = GetDC(hwnd);
    if (!hdc) return 0;

    int fmt = ChoosePixelFormat(hdc, &pfd);
    if (!fmt || !SetPixelFormat(hdc, fmt, &pfd)) return 0;

    hglrc = wglCreateContext(hdc);
    if (!hglrc || !wglMakeCurrent(hdc, hglrc)) return 0;

    return 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)nCmdShow;

    /* Windows passes /s, /p <hwnd>, or /c:<hwnd>; the separator varies. */
    char mode = 'c';
    HWND arg_hwnd = NULL;

    const char *p = lpCmdLine;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '/' || *p == '-') {
        p++;
        if (*p) mode = (char)tolower((unsigned char)*p++);
        while (*p == ':' || *p == ' ' || *p == '\t') p++;
        if (*p) arg_hwnd = (HWND)(UINT_PTR)strtoull(p, NULL, 0);
    }

    if (mode == 'c' || (mode != 's' && mode != 'p')) {
        MessageBoxA(arg_hwnd, "SoFi Gradient Cube has no configurable options.",
                    "SoFi Gradient Cube", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    is_preview = (mode == 'p');
    if (is_preview && !IsWindow(arg_hwnd)) return 1;

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = hInstance;
    wc.hCursor       = is_preview ? LoadCursor(NULL, IDC_ARROW) : NULL;
    wc.lpszClassName = "SoFiCubeSaver";
    if (!RegisterClassA(&wc)) return 1;

    HWND hwnd;
    if (is_preview) {
        RECT rc;
        GetClientRect(arg_hwnd, &rc);
        hwnd = CreateWindowExA(0, wc.lpszClassName, "", WS_CHILD | WS_VISIBLE,
                               0, 0, rc.right, rc.bottom,
                               arg_hwnd, NULL, hInstance, NULL);
    } else {
        int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        hwnd = CreateWindowExA(WS_EX_TOPMOST, wc.lpszClassName, "SoFi Gradient Cube",
                               WS_POPUP | WS_VISIBLE, x, y, w, h,
                               NULL, NULL, hInstance, NULL);
    }
    if (!hwnd) return 1;

    if (!init_gl_context(hwnd)) return 1;
    gl_ready = 1;

    start_ticks = GetTickCount64();
    cube_get_elapsed_ms = tick_elapsed_ms;
    QueryPerformanceFrequency(&qpc_freq);

    setup_gl();
    generate_rounded_cube();
    init_logo(asset_path("sofi.png"));
    init_font(font_path());

    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        resize(rc.right, rc.bottom);
    }

    if (!is_preview) {
        ShowCursor(FALSE);
        SetForegroundWindow(hwnd);
    }

    MSG msg;
    for (;;) {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto done;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        /* The preview pane outlives us only if its host is still alive. */
        if (is_preview && !IsWindow(arg_hwnd)) break;

        /*
         * SwapBuffers already blocks on vsync, so sleeping a further
         * TIMER_MS would halve the frame rate.  Sleep only the remainder,
         * which still paces the loop if a driver has vsync disabled.
         */
        LARGE_INTEGER t0, t1;
        QueryPerformanceCounter(&t0);
        render_frame();
        QueryPerformanceCounter(&t1);

        double spent_ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0
                        / (double)qpc_freq.QuadPart;
        if (spent_ms < TIMER_MS) Sleep((DWORD)(TIMER_MS - spent_ms));
    }

done:
    wglMakeCurrent(NULL, NULL);
    if (hglrc) wglDeleteContext(hglrc);
    if (hdc)   ReleaseDC(hwnd, hdc);
    if (!is_preview) ShowCursor(TRUE);

    return 0;
}

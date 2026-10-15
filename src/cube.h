#ifndef CUBE_H
#define CUBE_H

#ifdef __APPLE__
#  define GL_SILENCE_DEPRECATION
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#  ifndef SCREENSAVER_BUILD
#    include <GLUT/glut.h>
#  endif
#elif defined(_WIN32)
#  include <windows.h>
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/glut.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/glut.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_truetype.h"

/* ---------- constants ---------- */

#define FACE_COUNT      6
#define VERTS_PER_FACE  4
#define COUNTOF(a)      (sizeof(a) / sizeof((a)[0]))
#define TIMER_MS        16
#define STATS_INTERVAL  500
#define CAMERA_Z       -5.0f
#define FOV             45.0
#define SPIN_YAW        0.8f
#define SPIN_PITCH      0.3f

#define LOGO_FILE       "screensaver/Resources/sofi.png"
#define FONT_FILE       "screensaver/Resources/larsseit.ttf"
#define FONT_HEIGHT     18.0f
#define ATLAS_W         512
#define ATLAS_H         256
#define FIRST_CHAR      32
#define CHAR_COUNT      96

#define RC_RADIUS       0.12f
#define RC_SEG          8
#define RC_PI_2         1.5707963267948966f

/* ---------- types ---------- */

typedef struct { GLfloat r, g, b; } Color;

typedef struct {
    GLfloat pos[3];
    GLfloat nor[3];
    GLfloat uv[2];
    Color   col;
} RCVert;

/* ---------- shared state (extern) ---------- */

extern struct FontState {
    GLuint        texture;
    stbtt_bakedchar cdata[CHAR_COUNT];
    int           loaded;
} font;

extern struct LogoState {
    GLuint texture;
    int    loaded;
} logo;

extern struct RCMesh {
    RCVert *v;
    GLuint *idx;
    int     nv, ni;
    int     face_ni;
} rcmesh;

extern struct WinState {
    int w, h;
} win;

extern struct Stats {
    int   frames;
    int   prev_time;
    float fps;
    float frame_ms;
} stats;

extern const Color face_colors[FACE_COUNT][VERTS_PER_FACE];

extern int (*cube_get_elapsed_ms)(void);

/* ---------- function prototypes ---------- */

void init_font(const char *path);
void draw_text_stb(float x, float y, const char *str);
#ifndef SCREENSAVER_BUILD
void draw_text_glut(float x, float y, const char *str);
#endif

void init_logo(const char *path);

void generate_rounded_cube(void);

void draw_cube(void);
void update_stats(void);
void draw_stats_overlay(void);

#endif

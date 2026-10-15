#define STB_TRUETYPE_IMPLEMENTATION
#include "cube.h"

struct FontState font;

static unsigned char *read_file(const char *path, long *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = (unsigned char *)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    if ((long)fread(buf, 1, (size_t)sz, f) != sz) { free(buf); fclose(f); return NULL; }
    fclose(f);
    *out_size = sz;
    return buf;
}

void init_font(const char *path)
{
    long ttf_size = 0;
    unsigned char *ttf = read_file(path, &ttf_size);
    if (!ttf) {
        fprintf(stderr, "Warning: could not open %s -- falling back to GLUT bitmap font\n", path);
        font.loaded = 0;
        return;
    }

    unsigned char atlas[ATLAS_W * ATLAS_H];
    int ret = stbtt_BakeFontBitmap(ttf, 0, FONT_HEIGHT,
                                   atlas, ATLAS_W, ATLAS_H,
                                   FIRST_CHAR, CHAR_COUNT, font.cdata);
    free(ttf);
    if (ret <= 0) {
        fprintf(stderr, "Warning: stbtt_BakeFontBitmap failed -- falling back to GLUT bitmap font\n");
        font.loaded = 0;
        return;
    }

    glGenTextures(1, &font.texture);
    glBindTexture(GL_TEXTURE_2D, font.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, ATLAS_W, ATLAS_H, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    font.loaded = 1;
}

void draw_text_stb(float x, float y, const char *str)
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, font.texture);

    glBegin(GL_QUADS);
    for (const char *c = str; *c; c++) {
        int ch = (unsigned char)*c;
        if (ch < FIRST_CHAR || ch >= FIRST_CHAR + CHAR_COUNT) continue;
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(font.cdata, ATLAS_W, ATLAS_H,
                           ch - FIRST_CHAR, &x, &y, &q, 1);
        glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
        glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
        glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
        glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
    }
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

#ifndef SCREENSAVER_BUILD
void draw_text_glut(float x, float y, const char *str)
{
    glRasterPos2f(x, y);
    for (const char *c = str; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
}
#endif

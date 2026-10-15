#include "cube.h"

struct Stats stats;

void update_stats(void)
{
    stats.frames++;
    int now     = cube_get_elapsed_ms();
    int elapsed = now - stats.prev_time;
    if (elapsed >= STATS_INTERVAL) {
        stats.fps      = stats.frames * 1000.0f / (float)elapsed;
        stats.frame_ms = (float)elapsed / (float)stats.frames;
        stats.prev_time = now;
        stats.frames    = 0;
    }
}

void draw_stats_overlay(void)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, win.w, 0, win.h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);

    int elapsed_s = cube_get_elapsed_ms() / 1000;
    #define MAX_LINES 16
    char lines[MAX_LINES][48];
    int n = 0;
    snprintf(lines[n++], sizeof(lines[0]), "SoFi Spinning Cube");
    snprintf(lines[n++], sizeof(lines[0]), "FPS: %.1f",        stats.fps);
    snprintf(lines[n++], sizeof(lines[0]), "Frame: %.2f ms",   stats.frame_ms);
    snprintf(lines[n++], sizeof(lines[0]), "Verts: %d",        rcmesh.nv);
    snprintf(lines[n++], sizeof(lines[0]), "Tris: %d",         rcmesh.ni / 3);
    snprintf(lines[n++], sizeof(lines[0]), "Draw calls: 1");
    snprintf(lines[n++], sizeof(lines[0]), "Elapsed: %d:%02d", elapsed_s / 60, elapsed_s % 60);

    /*
     * stb_truetype uses top-left origin (y increases downward) while GLUT
     * bitmap fonts use bottom-left origin (y increases upward).  We set up
     * matching ortho projections for each path so line spacing works out.
     */
    if (font.loaded) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, win.w, win.h, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);

        float y = FONT_HEIGHT + 4.0f;
        for (int i = 0; i < n; i++, y += FONT_HEIGHT + 2.0f)
            draw_text_stb(8.0f, y, lines[i]);
    }
#ifndef SCREENSAVER_BUILD
    else {
        float y = (float)win.h - 16.0f;
        for (int i = 0; i < n; i++, y -= 14.0f)
            draw_text_glut(8.0f, y, lines[i]);
    }
#endif

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void draw_cube(void)
{
    if (logo.loaded) {
        /* Pass 1: textured logo on flat faces only */
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, logo.texture);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glColor3f(1.0f, 1.0f, 1.0f);

        {
            float uv_off = RC_RADIUS * 0.5f;
            float uv_scl = 1.0f / (1.0f - RC_RADIUS);
            glBegin(GL_TRIANGLES);
            for (int i = 0; i < rcmesh.face_ni; i++) {
                const RCVert *vt = &rcmesh.v[rcmesh.idx[i]];
                glNormal3fv(vt->nor);
                glTexCoord2f((vt->uv[0] - uv_off) * uv_scl,
                             (vt->uv[1] - uv_off) * uv_scl);
                glVertex3fv(vt->pos);
            }
            glEnd();
        }
        glDisable(GL_TEXTURE_2D);

        /* Pass 2: semi-transparent gradient overlay on flat faces */
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);

        glBegin(GL_TRIANGLES);
        for (int i = 0; i < rcmesh.face_ni; i++) {
            const RCVert *vt = &rcmesh.v[rcmesh.idx[i]];
            glNormal3fv(vt->nor);
            glColor4f(vt->col.r, vt->col.g, vt->col.b, 0.65f);
            glVertex3fv(vt->pos);
        }
        glEnd();

        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);

        /* Pass 3: solid metallic gradient on edges and corners */
        glBegin(GL_TRIANGLES);
        for (int i = rcmesh.face_ni; i < rcmesh.ni; i++) {
            const RCVert *vt = &rcmesh.v[rcmesh.idx[i]];
            glNormal3fv(vt->nor);
            glColor3f(vt->col.r, vt->col.g, vt->col.b);
            glVertex3fv(vt->pos);
        }
        glEnd();
    } else {
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < rcmesh.ni; i++) {
            const RCVert *vt = &rcmesh.v[rcmesh.idx[i]];
            glNormal3fv(vt->nor);
            glColor3f(vt->col.r, vt->col.g, vt->col.b);
            glVertex3fv(vt->pos);
        }
        glEnd();
    }
}

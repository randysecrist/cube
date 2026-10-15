#include "cube.h"

const Color face_colors[FACE_COUNT][VERTS_PER_FACE] = {
    /* front  */ {{0.05f,0.05f,0.07f},{0.25f,0.26f,0.30f},{0.50f,0.52f,0.55f},{0.08f,0.08f,0.10f}},
    /* back   */ {{0.04f,0.04f,0.06f},{0.15f,0.16f,0.20f},{0.35f,0.37f,0.42f},{0.06f,0.06f,0.08f}},
    /* top    */ {{0.28f,0.29f,0.32f},{0.50f,0.52f,0.55f},{0.62f,0.63f,0.66f},{0.18f,0.19f,0.22f}},
    /* bottom */ {{0.03f,0.03f,0.05f},{0.08f,0.08f,0.10f},{0.15f,0.15f,0.18f},{0.04f,0.04f,0.06f}},
    /* right  */ {{0.10f,0.10f,0.12f},{0.30f,0.31f,0.35f},{0.55f,0.56f,0.60f},{0.06f,0.06f,0.08f}},
    /* left   */ {{0.06f,0.06f,0.08f},{0.18f,0.19f,0.22f},{0.32f,0.33f,0.36f},{0.08f,0.09f,0.11f}},
};

struct RCMesh rcmesh;

static RCVert *rc_vbuf;
static GLuint *rc_ibuf;
static int     rc_nv, rc_ni;

static int rc_vert(const GLfloat p[3], const GLfloat n[3],
                   float u, float v, Color c)
{
    RCVert *vt = &rc_vbuf[rc_nv];
    vt->pos[0]=p[0]; vt->pos[1]=p[1]; vt->pos[2]=p[2];
    vt->nor[0]=n[0]; vt->nor[1]=n[1]; vt->nor[2]=n[2];
    vt->uv[0]=u; vt->uv[1]=v; vt->col=c;
    return rc_nv++;
}

static void rc_tri(int a, int b, int c)
{
    rc_ibuf[rc_ni++]=(GLuint)a;
    rc_ibuf[rc_ni++]=(GLuint)b;
    rc_ibuf[rc_ni++]=(GLuint)c;
}

static int rc_dom_face(const GLfloat n[3])
{
    float ax=fabsf(n[0]), ay=fabsf(n[1]), az=fabsf(n[2]);
    if (az>=ay && az>=ax) return n[2]>0 ? 0 : 1;
    if (ay>=ax)           return n[1]>0 ? 2 : 3;
    return                       n[0]>0 ? 4 : 5;
}

static void rc_face_uv(int f, const GLfloat p[3], float *u, float *v)
{
    switch (f) {
    case 0: *u=(p[0]+1)*0.5f; *v=(1-p[1])*0.5f; break;
    case 1: *u=(1-p[0])*0.5f; *v=(1-p[1])*0.5f; break;
    case 2: *u=(p[0]+1)*0.5f; *v=(p[2]+1)*0.5f;  break;
    case 3: *u=(p[0]+1)*0.5f; *v=(1-p[2])*0.5f;  break;
    case 4: *u=(1-p[2])*0.5f; *v=(1-p[1])*0.5f; break;
    case 5: *u=(p[2]+1)*0.5f; *v=(1-p[1])*0.5f; break;
    }
}

static Color rc_lerp_color(int f, float u, float v)
{
    const Color *c0=&face_colors[f][0], *c1=&face_colors[f][1];
    const Color *c2=&face_colors[f][2], *c3=&face_colors[f][3];
    Color c;
    c.r=(1-u)*(1-v)*c3->r + u*(1-v)*c2->r + (1-u)*v*c0->r + u*v*c1->r;
    c.g=(1-u)*(1-v)*c3->g + u*(1-v)*c2->g + (1-u)*v*c0->g + u*v*c1->g;
    c.b=(1-u)*(1-v)*c3->b + u*(1-v)*c2->b + (1-u)*v*c0->b + u*v*c1->b;
    return c;
}

static int rc_emit(const GLfloat p[3], const GLfloat n[3])
{
    int f = rc_dom_face(n);
    float u, v;
    rc_face_uv(f, p, &u, &v);
    return rc_vert(p, n, u, v, rc_lerp_color(f, u, v));
}

static void rc_grid_tris(const int *g, int rows, int cols, int flip)
{
    for (int j = 0; j < rows; j++)
        for (int i = 0; i < cols; i++) {
            int a=g[j*(cols+1)+i],     b=g[j*(cols+1)+i+1];
            int c=g[(j+1)*(cols+1)+i+1], d=g[(j+1)*(cols+1)+i];
            if (!flip) { rc_tri(a,b,c); rc_tri(a,c,d); }
            else       { rc_tri(a,c,b); rc_tri(a,d,c); }
        }
}

static int rc_check_flip(const int *g, int cols)
{
    int mid = (RC_SEG/2)*(cols+1) + RC_SEG/2;
    int v0=g[mid], v1=g[mid+1], v2=g[mid+cols+1];
    GLfloat e1[3], e2[3], cr[3];
    for (int k=0; k<3; k++) {
        e1[k] = rc_vbuf[v1].pos[k] - rc_vbuf[v0].pos[k];
        e2[k] = rc_vbuf[v2].pos[k] - rc_vbuf[v0].pos[k];
    }
    cr[0]=e1[1]*e2[2]-e1[2]*e2[1];
    cr[1]=e1[2]*e2[0]-e1[0]*e2[2];
    cr[2]=e1[0]*e2[1]-e1[1]*e2[0];
    float dot = cr[0]*rc_vbuf[v0].nor[0] +
                cr[1]*rc_vbuf[v0].nor[1] +
                cr[2]*rc_vbuf[v0].nor[2];
    return dot < 0.0f;
}

void generate_rounded_cube(void)
{
    const float r = RC_RADIUS, d = 1.0f - r;
    const int S = RC_SEG;

    static const int fax[6]={2,2,1,1,0,0};
    static const int fsg[6]={1,-1,1,-1,1,-1};
    static const int ft1[6]={0,0,2,2,1,1};
    static const int ft2[6]={1,1,0,0,2,2};

    int max_v = (6+12+8) * (S+1)*(S+1);
    int max_i = (6+12+8) * S*S * 6;
    rc_vbuf = (RCVert *)malloc((size_t)max_v * sizeof *rc_vbuf);
    rc_ibuf = (GLuint *)malloc((size_t)max_i * sizeof *rc_ibuf);
    rc_nv = rc_ni = 0;

    int grid[(RC_SEG+1)*(RC_SEG+1)];

    /* --- 6 flat faces --- */
    for (int f = 0; f < 6; f++) {
        int ax=fax[f], sg=fsg[f], t1=ft1[f], t2=ft2[f];
        for (int j = 0; j <= S; j++)
            for (int i = 0; i <= S; i++) {
                GLfloat p[3]={0,0,0}, n[3]={0,0,0};
                p[ax] = (float)sg;
                p[t1] = -d + 2.0f*d*(float)i/(float)S;
                p[t2] = -d + 2.0f*d*(float)j/(float)S;
                n[ax] = (float)sg;
                grid[j*(S+1)+i] = rc_emit(p, n);
            }
        rc_grid_tris(grid, S, S, rc_check_flip(grid, S));
    }

    int face_ni = rc_ni;

    /* --- 12 edges (quarter-cylinder strips) --- */
    for (int P = 0; P < 3; P++) {
        int A = (P == 0) ? 1 : 0;
        int B = (P <= 1) ? 2 : 1;
        for (int sA = -1; sA <= 1; sA += 2)
            for (int sB = -1; sB <= 1; sB += 2) {
                for (int j = 0; j <= S; j++)
                    for (int i = 0; i <= S; i++) {
                        float th = RC_PI_2 * (float)i / (float)S;
                        GLfloat p[3]={0,0,0}, n[3]={0,0,0};
                        p[P] = -d + 2.0f*d*(float)j/(float)S;
                        p[A] = (float)sA * (d + r*cosf(th));
                        p[B] = (float)sB * (d + r*sinf(th));
                        n[A] = (float)sA * cosf(th);
                        n[B] = (float)sB * sinf(th);
                        grid[j*(S+1)+i] = rc_emit(p, n);
                    }
                rc_grid_tris(grid, S, S, rc_check_flip(grid, S));
            }
    }

    /* --- 8 corners (eighth-sphere patches) --- */
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sy = -1; sy <= 1; sy += 2)
            for (int sz = -1; sz <= 1; sz += 2) {
                for (int j = 0; j <= S; j++) {
                    float th = RC_PI_2 * (float)j / (float)S;
                    for (int i = 0; i <= S; i++) {
                        float ph = RC_PI_2 * (float)i / (float)S;
                        GLfloat n[3], p[3];
                        n[0] = (float)sx * sinf(th) * cosf(ph);
                        n[1] = (float)sy * sinf(th) * sinf(ph);
                        n[2] = (float)sz * cosf(th);
                        p[0] = (float)sx*d + r*n[0];
                        p[1] = (float)sy*d + r*n[1];
                        p[2] = (float)sz*d + r*n[2];
                        grid[j*(S+1)+i] = rc_emit(p, n);
                    }
                }
                rc_grid_tris(grid, S, S, rc_check_flip(grid, S));
            }

    rcmesh.v       = rc_vbuf;
    rcmesh.idx     = rc_ibuf;
    rcmesh.nv      = rc_nv;
    rcmesh.ni      = rc_ni;
    rcmesh.face_ni = face_ni;
}

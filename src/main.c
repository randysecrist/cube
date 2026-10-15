#include "cube.h"

struct WinState win = {800, 600};

static int glut_elapsed(void) { return glutGet(GLUT_ELAPSED_TIME); }
int (*cube_get_elapsed_ms)(void) = glut_elapsed;

static struct {
    float pitch, yaw;
} rotation = {25.0f, 0.0f};

/* ---------- init ---------- */

static void init_gl(void)
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

/* ---------- GLUT callbacks ---------- */

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, CAMERA_Z);
    glRotatef(rotation.pitch, 1.0f, 0.0f, 1.0f);
    glRotatef(rotation.yaw,   0.0f, 1.0f, 1.0f);

    draw_cube();
    update_stats();
    draw_stats_overlay();

    glutSwapBuffers();
}

static void timer(int value)
{
    (void)value;
    rotation.yaw   += SPIN_YAW;
    rotation.pitch += SPIN_PITCH;
    if (rotation.yaw   > 360.0f) rotation.yaw   -= 360.0f;
    if (rotation.pitch > 360.0f) rotation.pitch -= 360.0f;
    glutPostRedisplay();
    glutTimerFunc(TIMER_MS, timer, 0);
}

static void reshape(int w, int h)
{
    if (h == 0) h = 1;
    win.w = w;
    win.h = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, (double)w / (double)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

static void keyboard(unsigned char key, int x, int y)
{
    (void)x; (void)y;
    if (key == 27 || key == 'q') exit(0);
}

/* ---------- main ---------- */

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(win.w, win.h);
    glutCreateWindow("Gradient Cube");

    init_gl();
    generate_rounded_cube();
    init_logo(LOGO_FILE);
    init_font(FONT_FILE);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(TIMER_MS, timer, 0);
    glutMainLoop();
    return 0;
}

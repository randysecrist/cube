#define SCREENSAVER_BUILD
#import "SoFICubeView.h"

#define GL_SILENCE_DEPRECATION
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>

#include "cube.h"

#import <CoreFoundation/CoreFoundation.h>

/* ---- globals normally provided by main.c ---- */

struct WinState win = {800, 600};
int (*cube_get_elapsed_ms)(void) = NULL;

/* ---- timing ---- */

static CFAbsoluteTime sStartTime;

static int cf_elapsed_ms(void)
{
    return (int)((CFAbsoluteTimeGetCurrent() - sStartTime) * 1000.0);
}

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

/* ---- ScreenSaverView ---- */

@implementation SoFICubeView
{
    NSOpenGLView *_glView;
    float _pitch;
    float _yaw;
    BOOL  _glReady;
}

- (instancetype)initWithFrame:(NSRect)frame isPreview:(BOOL)isPreview
{
    self = [super initWithFrame:frame isPreview:isPreview];
    if (!self) return nil;

    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAColorSize, 32,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
        0
    };
    NSOpenGLPixelFormat *fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (!fmt) return nil;

    _glView = [[NSOpenGLView alloc] initWithFrame:self.bounds pixelFormat:fmt];
    if (!_glView) return nil;

    _glView.wantsBestResolutionOpenGLSurface = YES;
    [_glView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [self addSubview:_glView];

    _pitch = 25.0f;
    _yaw   = 0.0f;

    [self setAnimationTimeInterval:1.0 / 60.0];
    return self;
}

- (void)startAnimation
{
    [super startAnimation];

    [[_glView openGLContext] makeCurrentContext];

    if (!_glReady) {
        sStartTime = CFAbsoluteTimeGetCurrent();
        cube_get_elapsed_ms = cf_elapsed_ms;

        setup_gl();
        generate_rounded_cube();

        NSBundle *bundle = [NSBundle bundleForClass:[self class]];
        NSString *logoPath = [bundle pathForResource:@"sofi" ofType:@"png"];
        NSString *fontPath = [bundle pathForResource:@"larsseit" ofType:@"ttf"];

        if (logoPath) init_logo([logoPath fileSystemRepresentation]);
        else          init_logo("");
        if (fontPath) init_font([fontPath fileSystemRepresentation]);
        else          init_font("");

        _glReady = YES;
    }

    [self reshapeGL];
}

- (void)stopAnimation
{
    [super stopAnimation];
}

- (void)reshapeGL
{
    [[_glView openGLContext] makeCurrentContext];

    NSSize px = [_glView convertSizeToBacking:_glView.bounds.size];
    int w = (int)px.width;
    int h = (int)px.height;
    if (h == 0) h = 1;

    win.w = w;
    win.h = h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, (double)w / (double)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize
{
    [super resizeSubviewsWithOldSize:oldSize];
    [_glView setFrame:self.bounds];
    if (_glReady) [self reshapeGL];
}

- (void)animateOneFrame
{
    [[_glView openGLContext] makeCurrentContext];

    _yaw   += SPIN_YAW;
    _pitch += SPIN_PITCH;
    if (_yaw   > 360.0f) _yaw   -= 360.0f;
    if (_pitch > 360.0f) _pitch -= 360.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, CAMERA_Z);
    glRotatef(_pitch, 1.0f, 0.0f, 1.0f);
    glRotatef(_yaw,   0.0f, 1.0f, 1.0f);

    draw_cube();

    if (![self isPreview]) {
        update_stats();
        draw_stats_overlay();
    }

    [[_glView openGLContext] flushBuffer];
}

- (BOOL)hasConfigureSheet { return NO; }
- (NSWindow *)configureSheet { return nil; }

@end

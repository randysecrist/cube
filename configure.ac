AC_PREREQ([2.69])
AC_INIT([cube], [1.0], [rsecrist@example.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

AC_PROG_CC

AC_CANONICAL_HOST

# --- Platform-specific OpenGL / GLUT detection ---

GL_CFLAGS=""
GL_LIBS=""
SCR_LIBS=""
build_win32_scr=no

case "$host_os" in
 darwin*)
 GL_LIBS="-framework OpenGL -framework GLUT"
 ;;
 mingw*|msys*|cygwin*)
 GL_LIBS="-lopengl32 -lglu32 -lfreeglut"
 # The .scr drives WGL directly, so it needs no GLUT.
 SCR_LIBS="-lopengl32 -lglu32 -lgdi32 -luser32"
 build_win32_scr=yes
 ;;
    *)
        # Linux / BSD / generic Unix
        AC_CHECK_LIB([m],    [cos],  [], [AC_MSG_ERROR([libm not found])])
        AC_CHECK_LIB([GL],   [glBegin],  [], [AC_MSG_ERROR([libGL not found])])
        AC_CHECK_LIB([GLU],  [gluPerspective], [], [AC_MSG_ERROR([libGLU not found])])
        AC_CHECK_LIB([glut], [glutInit], [], [AC_MSG_ERROR([libglut not found (install freeglut3-dev)])])
        GL_LIBS="-lGL -lGLU -lglut -lm"
        ;;
esac

AC_SUBST([GL_CFLAGS])
AC_SUBST([GL_LIBS])
AC_SUBST([SCR_LIBS])
AM_CONDITIONAL([BUILD_WIN32_SCR], [test "x$build_win32_scr" = xyes])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

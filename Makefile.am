bin_PROGRAMS = cube
cube_SOURCES = src/main.c src/font.c src/logo.c src/mesh.c src/render.c
cube_CFLAGS  = -std=c11 -Wall -Wextra -I$(srcdir) $(GL_CFLAGS)
cube_LDADD   = $(GL_LIBS)

if BUILD_WIN32_SCR
noinst_PROGRAMS   = cubesaver
cubesaver_SOURCES = src/win32_scr.c src/font.c src/logo.c src/mesh.c src/render.c
cubesaver_CFLAGS  = -std=c11 -Wall -Wextra -I$(srcdir) -DSCREENSAVER_BUILD $(GL_CFLAGS)
cubesaver_LDFLAGS = -mwindows
cubesaver_LDADD   = $(SCR_LIBS)

# Windows requires the .scr extension, but automake always appends $(EXEEXT).
all-local: cube.scr
cube.scr: cubesaver$(EXEEXT)
	cp -f $< $@
CLEANFILES = cube.scr
endif

DISTCLEANFILES = aclocal.m4 configure Makefile.in
distclean-local:
	rm -rf autom4te.cache build-aux

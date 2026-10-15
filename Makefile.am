bin_PROGRAMS = cube
cube_SOURCES = src/main.c src/font.c src/logo.c src/mesh.c src/render.c
cube_CFLAGS  = -std=c11 -Wall -Wextra -I$(srcdir) $(GL_CFLAGS)
cube_LDADD   = $(GL_LIBS)

DISTCLEANFILES = aclocal.m4 configure Makefile.in
distclean-local:
	rm -rf autom4te.cache build-aux

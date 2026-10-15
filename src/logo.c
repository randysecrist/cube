#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "cube.h"

struct LogoState logo;

void init_logo(const char *path)
{
    int w, h, channels;
    unsigned char *pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels) {
        fprintf(stderr, "Warning: could not load %s -- cube will have no logo\n", path);
        logo.loaded = 0;
        return;
    }

    glGenTextures(1, &logo.texture);
    glBindTexture(GL_TEXTURE_2D, logo.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(pixels);
    logo.loaded = 1;
}

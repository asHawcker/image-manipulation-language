#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

#include <iostream>
#include <cstdint>
#include <cstdlib>

struct ImgRuntime
{
    int32_t width;
    int32_t height;
    int32_t channels;
    uint8_t *data;
};

// extern "C" prevents C++ name mangling so LLVM can find these exactly by name
extern "C"
{
    void print(int x)
    {
        printf("%d\n", x);
    }
    void fprint(float x)
    {
        printf("%f\n", x);
    }
    void print_str(const char *s)
    {
        printf("%s\n", s);
    }

    ImgRuntime *load_image(const char *path)
    {
        int w, h, c;
        unsigned char *pixels = stbi_load(path, &w, &h, &c, 0);
        if (!pixels)
        {
            printf("Error loading image: %s\n", path);
            return nullptr;
        }

        ImgRuntime *img = (ImgRuntime *)malloc(sizeof(ImgRuntime));
        img->width = w;
        img->height = h;
        img->channels = c;
        img->data = pixels;
        return img;
    }

    void free_image(ImgRuntime *img)
    {
        if (img)
        {
            if (img->data)
            {
                stbi_image_free(img->data);
            }
            free(img);
        }
    }

    void print_img(ImgRuntime *img)
    {
        if (!img)
        {
            printf("Image(null)\n");
            return;
        }
        printf("Image [Width: %d, Height: %d, Channels: %d, DataPtr: %p]\n",
               img->width, img->height, img->channels, (void *)img->data);
    }
}

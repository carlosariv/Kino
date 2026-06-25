#pragma once

#include <string>

#include "base_types.h"
#include "string.h"
// #include "render.h"

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;

enum TextureFormat {
    R8_UNorm,
    R8G8B8A8_UNorm,
};

struct Texture {
    int width;
    int height;
    TextureFormat format;
    void *data;

    ID3D11Texture2D *tex2d;
    ID3D11ShaderResourceView *srv;
};

usize get_texture_format_bytes(TextureFormat format);
Texture *texture_create(String filename, TextureFormat format = TextureFormat::R8G8B8A8_UNorm);
Texture *texture_create(void *data, int width, int height, TextureFormat format = TextureFormat::R8G8B8A8_UNorm);

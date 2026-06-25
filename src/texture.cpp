#include <stb/stb_image.h>
#include <d3d11.h>

#include "texture.h"
#include "render.h"

usize get_texture_format_bytes(TextureFormat format) {
    switch (format) {
        case R8_UNorm:
            return 1;
        case R8G8B8A8_UNorm:
            return 4;
        default:
            return 1;
    }
}

DXGI_FORMAT texture_format_to_dxgi(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8_UNorm:
            return DXGI_FORMAT_R8_UNORM;
        case TextureFormat::R8G8B8A8_UNorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

Texture *texture_create(String filename, TextureFormat format) {
    int width, height, n;
    usize bytes_per_pixel = get_texture_format_bytes(format);
    u8 *bitmap = stbi_load((char *)filename.text, &width, &height, &n, bytes_per_pixel);
    if (!bitmap) {
        fprintf(stderr, "ERROR::TEXTURE Could not load file '{}'", (char *)filename.text);
        return nullptr;
    }
    // stbi_image_free(bitmap);
    return texture_create(bitmap, width, height, format);
}

Texture *texture_create(void *data, int width, int height, TextureFormat format) {
    Texture *texture = new Texture();
    texture->data = data;
    texture->width = width;
    texture->height = height;
    texture->format = format;

    usize bytes_per_pixel = get_texture_format_bytes(format);

    //TODO: Think about flags for dynamic textures and mipmapping

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = texture_format_to_dxgi(format);
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    ID3D11Texture2D *texture_2d = NULL;
    D3D11State *d3d11_state = get_d3d11_state();
    D3D11_SUBRESOURCE_DATA initial_data = {};
    initial_data.pSysMem = data;
    initial_data.SysMemPitch = width * bytes_per_pixel;
    initial_data.SysMemSlicePitch = 0;
    HRESULT hr = d3d11_state->device->CreateTexture2D(&desc, &initial_data, &texture_2d);
    if (hr != S_OK) {
        printf("D3D11: CreateTexture2D failed");
    }

    ID3D11ShaderResourceView *srv = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = desc.MipLevels - 1;
    srv_desc.Texture2D.MipLevels = 1;
    if (FAILED(d3d11_state->device->CreateShaderResourceView(texture_2d, &srv_desc, &srv))) {
        printf("D3D11: CreateShaderResourceView failed");
    }

    texture->tex2d = texture_2d;
    texture->srv = srv;

    return texture;
}

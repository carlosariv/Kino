#pragma once

#include <unordered_map>
#include <string>

#include "shader.h"
#include "vector.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11_1.h>

// #include <D3Dcommon.h>

struct D3D11State {
    IDXGIFactory2 *dxgi_factory = nullptr;
    IDXGISwapChain1 *swap_chain = nullptr;
    ID3D11Device1 *device = nullptr;
    ID3D11DeviceContext1 *device_context = nullptr;
    ID3D11RenderTargetView *render_target = nullptr;
    ID3D11DepthStencilView *depth_stencil_view = nullptr;
    ID3D11Texture2D *depth_stencil_texture = nullptr;
};

struct RenderState {
    Vector2 last_resolution = {-1, -1};
    std::unordered_map<String, Shader*, StringHasher> shaders;

    void add_shader(Shader *shader);
    Shader *get_shader(String name);
};


ID3D11Buffer *vertex_buffer_create(void *memory, uint elem_size, uint elem_count);

void render_init();
void render_frame(Vector2 dimension);

RenderState *get_render_state();
D3D11State *get_d3d11_state();

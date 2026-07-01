#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>

#include <queue>

#include "base_types.h"
#include "math.h"
#include "shader.h"
#include "render.h"
#include "texture.h"
#include "camera.h"
#include "font.h"
#include "ui.h"
#include "profiler.h"

D3D11State *g_d3d11_state;
RenderState *g_render_state;

Texture *atlas_texture;

ID3D11Buffer *ui_cbuffer;

ID3D11RasterizerState *cull_none;
ID3D11RasterizerState *rasterizer_ui;
ID3D11RasterizerState *cull_back;

ID3D11BlendState *blend_state_ui;

ID3D11SamplerState *sampler_point;

ID3D11DepthStencilState *no_depth_state;

ID3D11Buffer *chunk_constant_buffer;

RenderState *get_render_state() {
    return g_render_state;
}

D3D11State *get_d3d11_state() {
    return g_d3d11_state;
}

Shader *RenderState::get_shader(String name) {
    auto found = shaders.find(name);
    if (found != shaders.end()) {
        return found->second;
    }
    return nullptr;
}

void RenderState::add_shader(Shader *shader) {
    shaders.insert({shader->name, shader});
}

void d3d11_on_resize(HWND hWnd, int width, int height) {
    D3D11State *d3d11_state = g_d3d11_state;
    assert(d3d11_state->swap_chain);

    if (width <= 0) width = 1;
    if (height <= 0) height = 1;

    d3d11_state->device_context->OMSetRenderTargets(0, nullptr, nullptr);

    if (d3d11_state->render_target) {
        d3d11_state->render_target->Release();
    }

    HRESULT hr;
    hr = d3d11_state->swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    if (FAILED(hr)) {
        spdlog::error("D3D11: ResizeBuffers failed {} {}", width, height);
    }

    ID3D11Texture2D *back_buffer = nullptr;
    hr = d3d11_state->swap_chain->GetBuffer(0, __uuidof( ID3D11Texture2D), (void**)&back_buffer);
    if (FAILED(hr)) {
        spdlog::error("D3D11: GetBuffer failed");
    }

    D3D11_TEXTURE2D_DESC backbuffer_desc;
    back_buffer->GetDesc(&backbuffer_desc);

    D3D11_RENDER_TARGET_VIEW_DESC framebuffer_desc = {};
    framebuffer_desc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
    framebuffer_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    hr = d3d11_state->device->CreateRenderTargetView(back_buffer, &framebuffer_desc, &d3d11_state->render_target);
    if (FAILED(hr)) {
        spdlog::error("D3D11: CreateRenderTargetView failed");
    }

    // width = backbuffer_desc.Width;
    // height = backbuffer_desc.Height;

    g_render_state->last_resolution = Vector2(width, height);

    back_buffer->Release();

    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    d3d11_state->device_context->RSSetViewports(1, &vp);

    if (0) {
        if (d3d11_state->depth_stencil_texture) {
            d3d11_state->depth_stencil_texture->Release();
            d3d11_state->depth_stencil_texture = nullptr;
        }

        if (d3d11_state->depth_stencil_view) {
            // ID3D11Resource* pResource = nullptr;
            // d3d11_state->depth_stencil_view->GetResource(&pResource);
            //
            // // 2. Query the resource for the 2D Texture interface
            // ID3D11Texture2D* pDepthTexture = nullptr;
            // hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pDepthTexture);
            // assert(pDepthTexture);
            //
            // // 3. Clean up the temporary resource pointer to prevent memory leaks
            // pResource->Release();
            //
            // if (SUCCEEDED(hr)) {
            //     pDepthTexture->Release();
            // }
            d3d11_state->depth_stencil_view->Release();
            d3d11_state->depth_stencil_view = nullptr;
        }


        D3D11_TEXTURE2D_DESC depth_desc;
        depth_desc.Width = width;
        depth_desc.Height = height;
        depth_desc.MipLevels = 1;
        depth_desc.ArraySize = 1;
        depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_desc.SampleDesc.Count = 1; // @Note: Must match Render Target
        depth_desc.SampleDesc.Quality = 0;
        depth_desc.Usage = D3D11_USAGE_DEFAULT;
        depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depth_desc.CPUAccessFlags = 0;
        depth_desc.MiscFlags = 0;
        hr = d3d11_state->device->CreateTexture2D(&depth_desc, NULL, &d3d11_state->depth_stencil_texture);

        D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
        dsv_desc.Format = depth_desc.Format;
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Texture2D.MipSlice = 0;
        hr = d3d11_state->device->CreateDepthStencilView(d3d11_state->depth_stencil_texture, &dsv_desc, &d3d11_state->depth_stencil_view);
    }

    // d3d11_state->device_context->OMSetRenderTargets(1, &d3d11_state->render_target, d3d11_state->depth_stencil_view);
}

void render_state_init() {
    RenderState *render_state = new RenderState();
    g_render_state = render_state;
}

void d3d11_init(HWND hWnd) {
    D3D11State *d3d11_state = new D3D11State();
    g_d3d11_state = d3d11_state;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    UINT create_device_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ID3D11Device* base_device = nullptr;
    ID3D11DeviceContext* base_device_context = nullptr;

    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &base_device, nullptr, &base_device_context);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Device1* device = nullptr;

    base_device->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&device));

    ID3D11DeviceContext1* device_context = nullptr;

    base_device_context->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&device_context));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    IDXGIDevice1* dxgi_device = nullptr;

    device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&dxgi_device));

    IDXGIAdapter* dxgi_adapter = nullptr;

    dxgi_device->GetAdapter(&dxgi_adapter);

    IDXGIFactory2* dxgi_factory = nullptr;

    dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgi_factory));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width            = 0; // use window width
    swap_chain_desc.Height           = 0; // use window height
    swap_chain_desc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM; // can't specify _SRGB here when using DXGI_SWAP_EFFECT_FLIP_* ...
    swap_chain_desc.Stereo           = FALSE;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount      = 2;
    swap_chain_desc.Scaling          = DXGI_SCALING_NONE;
    swap_chain_desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.AlphaMode        = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc.Flags            = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    IDXGISwapChain1* swap_chain = nullptr;

    dxgi_factory->CreateSwapChainForHwnd(device, hWnd, &swap_chain_desc, nullptr, nullptr, &swap_chain);

    d3d11_state->dxgi_factory = dxgi_factory;
    d3d11_state->swap_chain = swap_chain;
    d3d11_state->device = device;
    d3d11_state->device_context = device_context;
}


void render_init() {
    D3D11_INPUT_ELEMENT_DESC ui_ilay[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    Shader *rect_ui_shader = shader_create(STRZ("RectUI"), STRZ("shaders/RectUI.hlsl"), ui_ilay, cu_count_of(ui_ilay));


    D3D11_INPUT_ELEMENT_DESC chunk_ilay[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    Shader *chunk_shader = shader_create(STRZ("Chunk"), STRZ("shaders/chunk.hlsl"), chunk_ilay, cu_count_of(chunk_ilay));

    {
        Matrix4 proj = math::orthographic_rh_no(0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f);
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth = sizeof(Matrix4);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA init_data;
        init_data.pSysMem = &proj;
        init_data.SysMemPitch = 0;
        init_data.SysMemSlicePitch = 0;

        HRESULT hr = g_d3d11_state->device->CreateBuffer(&desc, &init_data, &ui_cbuffer);
        if (FAILED(hr)) {
            printf("D3D11: Failed to create constant buffer.");
        }
    }


    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_BACK;
        desc.FrontCounterClockwise = FALSE;
        desc.AntialiasedLineEnable = TRUE;
        desc.ScissorEnable = TRUE;
        g_d3d11_state->device->CreateRasterizerState(&desc, &rasterizer_ui);
    }


    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.FrontCounterClockwise = TRUE;
        desc.CullMode = D3D11_CULL_NONE;
        g_d3d11_state->device->CreateRasterizerState(&desc, &cull_none);
    }

    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.FrontCounterClockwise = TRUE;
        desc.CullMode = D3D11_CULL_BACK;
        g_d3d11_state->device->CreateRasterizerState(&desc, &cull_back);
    }

    // DEPTH STENCIL STATE
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.StencilEnable = FALSE;
        g_d3d11_state->device->CreateDepthStencilState(&desc, &no_depth_state);
    }

    // BLEND STATE
    {
        D3D11_RENDER_TARGET_BLEND_DESC rtd = {};
        rtd.BlendEnable = true;
        rtd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
        rtd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        rtd.BlendOp = D3D11_BLEND_OP_ADD;
        rtd.SrcBlendAlpha = D3D11_BLEND_ONE;
        rtd.DestBlendAlpha = D3D11_BLEND_ZERO;
        rtd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rtd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        D3D11_BLEND_DESC desc = {};
        desc.RenderTarget[0] = rtd;
        HRESULT hr = g_d3d11_state->device->CreateBlendState(&desc, &blend_state_ui);
        if (FAILED(hr)) {
            printf("D3D11: Failed to create blend state.");
        }
    }

    // SAMPLER STATE
    {
        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        HRESULT hr = g_d3d11_state->device->CreateSamplerState(&desc, &sampler_point);
        if (FAILED(hr)) {
            printf("D3D11: Failed to create sampler state.");
        }
    }

    // D3D11_INPUT_ELEMENT_DESC ui_ilay[] = {
    //     { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //     { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //     { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    // };
    //
    // Shader *basic2d_shader = shader_create("Basic2D", "shaders/basic2d.hlsl", }));
    //
}

ID3D11Buffer *vertex_buffer_create(void *memory, uint elem_size, uint elem_count) {
    D3D11_BUFFER_DESC buffer_desc;
    buffer_desc.Usage            = D3D11_USAGE_IMMUTABLE;
    buffer_desc.ByteWidth        = elem_size * elem_count;
    buffer_desc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
    buffer_desc.CPUAccessFlags   = 0;
    buffer_desc.MiscFlags        = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = memory;
    data.SysMemPitch = 0;
    data.SysMemSlicePitch = 0;

    ID3D11Buffer *vertex_buffer = nullptr;
    HRESULT hr = g_d3d11_state->device->CreateBuffer(&buffer_desc, &data, &vertex_buffer);
    if (FAILED(hr)) {
        spdlog::error("Failed to create vertex buffer! data:{} bytes:{}", memory, buffer_desc.ByteWidth);
    }
    return vertex_buffer;
}

ID3D11Buffer *vertex_buffer_create(uint elem_size, uint elem_count) {
    D3D11_BUFFER_DESC buffer_desc;
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = elem_size * elem_count;
    buffer_desc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    ID3D11Buffer *vertex_buffer = nullptr;
    HRESULT hr = g_d3d11_state->device->CreateBuffer(&buffer_desc, NULL, &vertex_buffer);
    if (FAILED(hr)) {
        spdlog::error("Failed to create vertex buffer! bytes:{}", buffer_desc.ByteWidth);
    }
    return vertex_buffer;
}

void render_ui(ui::DrawData *draw_data, Vector2 render_dimension) {
    D3D11State *d3d11 = g_d3d11_state;
    Shader *shader = g_render_state->get_shader(STRZ("RectUI"));

    Matrix4 xform = math::orthographic_rh_no(0.0f, render_dimension.x, render_dimension.y, 0.0f, -1.0f, 1.0f);
    {
        D3D11_MAPPED_SUBRESOURCE mapped_res;
        d3d11->device_context->Map(ui_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res);
        memcpy(mapped_res.pData, (void *)&xform, sizeof(xform));
        d3d11->device_context->Unmap(ui_cbuffer, 0);
    }

    d3d11->device_context->VSSetShader(shader->vertex_shader, nullptr, 0);
    d3d11->device_context->PSSetShader(shader->pixel_shader, nullptr, 0);
    d3d11->device_context->IASetInputLayout(shader->input_layout);
    d3d11->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //TODO: Update a dynamic resizable vertex buffer instead of creating new one every frame
    if (draw_data->vertex_buffer) {
        draw_data->vertex_buffer->Release();
    }
    draw_data->vertex_buffer = vertex_buffer_create(draw_data->vertices.data, sizeof(ui::Vertex), draw_data->vertices.count);

    uint stride = sizeof(ui::Vertex);
    uint offset = 0;
    d3d11->device_context->IASetVertexBuffers(0, 1, &draw_data->vertex_buffer, &stride, &offset);
    d3d11->device_context->VSSetConstantBuffers(0, 1, &ui_cbuffer);

    d3d11->device_context->OMSetDepthStencilState(no_depth_state, 0);

    FLOAT blend_factor[4] = {0, 0, 0, 0};
    d3d11->device_context->OMSetBlendState(blend_state_ui, blend_factor, 0xFFFFFFFF);
    d3d11->device_context->PSSetSamplers(0, 1, &sampler_point);

    d3d11->device_context->RSSetState(rasterizer_ui);

    for (ui::DrawBatch *batch : draw_data->batches) {
        D3D11_RECT clip_rect = {(int)batch->clip.x0, (int)batch->clip.y0, (int)batch->clip.x1, (int)batch->clip.y1};
        d3d11->device_context->RSSetScissorRects(1, &clip_rect);
        d3d11->device_context->PSSetShaderResources(0, 1, &batch->texture->srv);
        d3d11->device_context->Draw(batch->vertex_count, batch->vertices_index);
    }

    d3d11->device_context->OMSetDepthStencilState(nullptr, 0);
    d3d11->device_context->OMSetBlendState(nullptr, NULL, 0xFFFFFFFF);
    d3d11->device_context->PSSetSamplers(0, 0, nullptr);
}

// void render_chunks(Vector2 render_dimension, GameState *game_state) {
//     Camera *camera = game_state->camera;
//     D3D11State *d3d11 = get_d3d11_state();
//
//     Shader *shader = g_render_state->get_shader(STRZ("Chunk"));
//
//     d3d11->device_context->VSSetShader(shader->vertex_shader, nullptr, 0);
//     d3d11->device_context->PSSetShader(shader->pixel_shader, nullptr, 0);
//     d3d11->device_context->IASetInputLayout(shader->input_layout);
//     d3d11->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//     d3d11->device_context->PSSetShaderResources(0, 1, &atlas_texture->srv);
//     d3d11->device_context->PSSetSamplers(0, 1, &sampler_point);
//
//     d3d11->device_context->RSSetState(cull_back);
//
//     ChunkManager *chunk_manager = game_state->chunk_manager;
//
//     cull_chunks(chunk_manager, camera);
//
//     ChunkConstants chunk_constants;
//     chunk_constants.proj = camera->projection_matrix;
//     chunk_constants.view = camera->view_matrix;
//
//     for (Chunk *chunk : chunk_manager->render_list) {
//         if (chunk->vertices.count == 0) continue;
//         Vector3 min = chunk->position * CHUNK_SIZE;
//         Vector3 max = min + Vector3(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
//         Matrix4 model = math::translate(chunk->position * CHUNK_SIZE);
//         chunk_constants.model = model;
//
//         {
//             D3D11_MAPPED_SUBRESOURCE mapped_res;
//             d3d11->device_context->Map(chunk_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res);
//             memcpy(mapped_res.pData, (void *)&chunk_constants, sizeof(chunk_constants));
//             d3d11->device_context->Unmap(chunk_constant_buffer, 0);
//         }
//
//         if (chunk->vertex_buffer_capacity < chunk->vertices.count) {
//             if (chunk->vertex_buffer) ((ID3D11Buffer*)chunk->vertex_buffer)->Release();
//             chunk->vertex_buffer = nullptr;
//         }
//
//         if (chunk->vertex_buffer == nullptr) {
//             chunk->vertex_buffer_capacity = chunk->vertices.count;
//             chunk->vertex_buffer = vertex_buffer_create(sizeof(ChunkVertex), chunk->vertices.count);
//         }
//
//         ID3D11Buffer *vertex_buffer = (ID3D11Buffer *)chunk->vertex_buffer;
//         if (chunk->buffer_dirty) {
//             D3D11_MAPPED_SUBRESOURCE mapped_res;
//             d3d11->device_context->Map(vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res);
//             memcpy(mapped_res.pData, chunk->vertices.data, chunk->vertices.count * sizeof(ChunkVertex));
//             d3d11->device_context->Unmap(vertex_buffer, 0);
//             chunk->buffer_dirty = false;
//         }
//
//         uint stride = sizeof(ChunkVertex);
//         uint offset = 0;
//         d3d11->device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
//         d3d11->device_context->VSSetConstantBuffers(0, 1, &chunk_constant_buffer);
//
//         d3d11->device_context->Draw(chunk->vertices.count, 0);
//     }
// }

void render_frame(Vector2 render_dimension) {
    D3D11State *d3d11_state = g_d3d11_state;

    //- NOTE: Do not render when minimized.
    if (os::main_window->is_minimized) {
        return;
    }

    bool resize_done = false;
    if (render_dimension.x != g_render_state->last_resolution.x || render_dimension.y != g_render_state->last_resolution.y) {
        HWND hWnd = (HWND)os::main_window->handle;
        d3d11_on_resize(hWnd, render_dimension.x, render_dimension.y);
        resize_done = true;
    }


    float framebuffer_clear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    d3d11_state->device_context->ClearRenderTargetView(d3d11_state->render_target, framebuffer_clear);
    if (resize_done) {
        d3d11_state->device_context->Flush();
    }

    if (d3d11_state->depth_stencil_view) {
        d3d11_state->device_context->ClearDepthStencilView(d3d11_state->depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    d3d11_state->device_context->OMSetRenderTargets(1, &d3d11_state->render_target, d3d11_state->depth_stencil_view);

    f32 aspect_ratio = render_dimension.x / render_dimension.y;
    Matrix4 screen_projection = math::orthographic_rh_no(0.0f, render_dimension.x, 0.0f, render_dimension.y, -1.0f, 1.0f);
    Matrix4 projection = math::perspective_rh_no(math::PI * 0.5f, aspect_ratio, 0.001f, 1000.0f);
    Matrix4 scale = math::scale(100.0f, 100.0f, 1.0f);
    Matrix4 offset = math::translate(Vector3(0.5f * render_dimension.x, 0.5f * render_dimension.y, 0.0f));
    Matrix4 transform = projection * offset * scale;

    render_ui(ui::ui_draw_data, render_dimension);

    d3d11_state->swap_chain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

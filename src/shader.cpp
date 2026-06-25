#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <iterator>

#include "render.h"
#include "shader.h"

Shader *shader_create(String name, String file_path, D3D11_INPUT_ELEMENT_DESC *ilay_desc, int ilay_desc_count) {
    Shader *shader = new Shader();
    shader->name = name;
    shader->full_path = file_path;

    int compilation_success = true;

    os::Handle file_handle = os::open_file(file_path, os::FileAccessFlag_Read|os::FileAccessFlag_Open);
    String contents = {};
    if (os::is_valid_handle(file_handle)) {
        u64 last_write_time = os::get_file_last_write_time(file_handle);
        shader->last_write_time = last_write_time;
        contents = os::read_entire_file(file_handle);
        os::close_handle(file_handle);
    } else {
        compilation_success = false;
        printf("Could not read file '{}' for shader compilation.", file_path);
        return nullptr;
    }

    UINT compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob *vs_blob = nullptr, *ps_blob = nullptr;
    ID3DBlob *vs_error = nullptr, *ps_error = nullptr;

    if (FAILED(D3DCompile(contents.text, contents.len, (char *)file_path.text, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs_main", "vs_5_0", compile_flags, NULL, &vs_blob, &vs_error))) {
        compilation_success = false;
        spdlog::error("D3D11: Failed to load vertex shader from file {}", (char *)file_path.text);
        if (vs_error) {
            spdlog::error("D3D11: With message: {}", static_cast<const char*>(vs_error->GetBufferPointer()));
        }
    }

    if (FAILED(D3DCompile(contents.text, contents.len, (char *)file_path.text, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps_main", "ps_5_0", compile_flags, NULL, &ps_blob, &ps_error))) {
        compilation_success = false;
        spdlog::error("D3D11: Failed to load shader from file {}", (char *)file_path.text);
        if (ps_error) {
            spdlog::error("D3D11: With message: {}", static_cast<const char*>(ps_error->GetBufferPointer()));
        }
    }

    RenderState *render_state = get_render_state();
    D3D11State *d3d11_state = get_d3d11_state();

    ID3D11VertexShader *vertex_shader = nullptr;
    if (FAILED(d3d11_state->device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &vertex_shader))) {
        printf("D3D11: Failed to create vertex shader");
        compilation_success = false;
    }

    ID3D11PixelShader *pixel_shader = nullptr;
    if (FAILED(d3d11_state->device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &pixel_shader))) {
        printf("D3D11: Failed to create pixel shader");
        compilation_success = false;
    }

    ID3D11InputLayout *input_layout = nullptr;
    if (FAILED(d3d11_state->device->CreateInputLayout(ilay_desc, ilay_desc_count, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &input_layout))) {
        printf("D3D11: Failed to create input layout");
        compilation_success = false;
    }

    shader->vertex_shader = vertex_shader;
    shader->pixel_shader = pixel_shader;
    shader->input_layout = input_layout;

    render_state->add_shader(shader);
    return shader;
}

#pragma once

#include <string>
#include <filesystem>
#include <d3d11.h>

#include "base_types.h"
#include "os.h"

struct Shader {
    String name;
    String full_path;
    uint program;
    os::Handle file_handle;
    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3D11InputLayout *input_layout;

    u64 last_write_time;

    Shader() = default;
};


Shader *shader_create(String name, String file_path, D3D11_INPUT_ELEMENT_DESC *ilay_desc, int ilay_desc_count);

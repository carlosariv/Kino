
struct VSIn {
    float2 position : POSITION;
    float2 uv : TEXCOORD;

    float4 color : COLOR;
};

struct VSOut {
    float4 position : SV_POSITION;
    float4 color : COL;
    float2 uv : TEX;
};

cbuffer Constants {
    float4x4 xform;
};

Texture2D main_texture;
SamplerState main_sampler;

VSOut vs_main(VSIn input) {
    VSOut output;
    output.position = mul(xform, float4(input.position, 0, 1));
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 ps_main(VSOut input) : SV_TARGET {
    float tex_alpha = main_texture.Sample(main_sampler, input.uv).r;
    float4 final_color;
    final_color = input.color;
    final_color.a *= tex_alpha;
    // float4 sampled = float4(1, 1, 1, tex_alpha);
    // final_color = float4(input.color.rgb, 1.0) * sampled;
    return final_color;
}

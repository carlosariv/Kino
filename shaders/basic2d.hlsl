
struct VSInput {
    float2 position : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COL;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COL;
};

cbuffer Constants : register(b0) {
    float4x4 xform;
};

SamplerState MainTextureSampler {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

Texture2D main_texture;

PSInput vs_main(VSInput input) {
    PSInput output;
    output.position = mul(xform, float4(input.position, 0, 1));
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

float4 ps_main(PSInput input) : SV_TARGET {
    float4 final_color = main_texture.Sample(MainTextureSampler, input.uv) * input.color;
    return final_color;
}

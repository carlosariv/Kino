struct VSInput {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

cbuffer Constants : register(b0) {
    float4x4 proj;
    float4x4 view;
    float4x4 model;
};

Texture2D main_texture;
SamplerState main_sampler;

PSInput vs_main(VSInput input) {
    PSInput output;
    output.position = mul(proj, mul(view, mul(model, float4(input.position, 1.0))));
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 ps_main(PSInput input) : SV_TARGET {
    float4 texture_color = main_texture.Sample(main_sampler, input.uv);
    float4 final_color = input.color * texture_color;
    // final_color.xyz = input.position.xyz / input.position.w / 200;
    return final_color;
}

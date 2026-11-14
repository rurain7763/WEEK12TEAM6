#pragma pack_matrix(row_major)

struct ObjectInstance
{
    matrix Model;
    matrix NormalMatrix;
    float4 Color;
};

cbuffer constants : register(b0)
{
    matrix VP;
};

Texture2D SpriteTexture : register(t0);
StructuredBuffer<ObjectInstance> ObjectInstances : register(t1);

SamplerState SpriteSampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 Color : COLOR0;
    float2 uv : TEXCOORD0;
};

PS_INPUT mainVS(VS_INPUT input, uint InstanceID : SV_InstanceID)
{
    PS_INPUT output;

    ObjectInstance Instance = ObjectInstances[InstanceID];

    output.position = mul(mul(float4(input.position, 1.0f), Instance.Model), VP);
    output.Color = Instance.Color;
    output.uv = input.uv;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 color = SpriteTexture.Sample(SpriteSampler, input.uv);
    color *= input.Color;

    clip(color.a - 0.01f);

    return color;
}

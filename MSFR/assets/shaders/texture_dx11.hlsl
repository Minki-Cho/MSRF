cbuffer PerDraw : register(b0)
{
    float4x4 uModelToNDC;
    float2 uTexelPos;
    float2 uFrameSize;
};

Texture2D uTex : register(t0);
SamplerState uSamp : register(s0);

struct VSIn
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOut VSMain(VSIn v)
{
    VSOut o;
    o.pos = mul(uModelToNDC, float4(v.pos, 0, 1));
    o.uv = v.uv;
    return o;
}

float4 PSTexture(VSOut i) : SV_TARGET
{
    return uTex.Sample(uSamp, i.uv);
}

float4 PSTexel(VSOut i) : SV_TARGET
{
    float2 uv = uTexelPos + i.uv * uFrameSize;
    return uTex.Sample(uSamp, uv);
}

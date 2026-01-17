cbuffer PerDraw : register(b0)
{
    float4x4 uModelToNDC;
};

struct VSIn
{
    float2 pos : POSITION;
    float3 col : COLOR;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

VSOut VSMain(VSIn v)
{
    VSOut o;
    float4 p = float4(v.pos.xy, 0.0f, 1.0f);
    o.pos = mul(uModelToNDC, p);
    o.col = v.col;
    return o;
}

float4 PSMain(VSOut i) : SV_TARGET
{
    return float4(i.col, 1.0f);
}

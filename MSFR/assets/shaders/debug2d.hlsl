cbuffer PerDraw : register(b0)
{
    row_major float4x4 uModelToNDC;
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
    o.pos = mul(uModelToNDC, float4(v.pos, 0, 1));
    o.col = v.col;
    return o;
}

float4 PSMain(VSOut i) : SV_TARGET
{
    return float4(i.col, 1.0);
}
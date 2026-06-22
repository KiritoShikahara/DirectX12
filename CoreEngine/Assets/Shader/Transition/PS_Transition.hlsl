// PS_Transition.hlsl
// Root32BitConstants (b0) : float4(R, G, B, A)

struct VSOutput
{
    float4 Position : SV_Position;
};

cbuffer TransitionCB : register(b0)
{
    float4 gColor; // r, g, b, a
};

float4 main(VSOutput input) : SV_Target
{
    return float4(gColor.rgb, gColor.a);
}

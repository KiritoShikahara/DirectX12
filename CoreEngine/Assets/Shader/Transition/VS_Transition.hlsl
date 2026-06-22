// VS_Transition.hlsl
// 頂点バッファなし：SV_VertexID から NDC 座標を直接生成する
// DrawInstanced(3, 1, 0, 0) で画面全体を覆う巨大三角形を描画

struct VSOutput
{
    float4 Position : SV_Position;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    // vertexID  UV               NDC
    //    0    (0, 0)   →   (-1,  1)  左上
    //    1    (2, 0)   →   ( 3,  1)  右上（画面外）
    //    2    (0, 2)   →   (-1, -3)  左下（画面外）
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);

    VSOutput output;
    output.Position = float4(uv.x * 2.0f - 1.0f, -(uv.y * 2.0f - 1.0f), 0.0f, 1.0f);
    return output;
}

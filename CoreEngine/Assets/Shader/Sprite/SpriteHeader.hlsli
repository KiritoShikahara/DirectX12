
struct SpriteShaderData
{
    float4x4 WVP; // ワールド × 正射影行列（転置済み）
    float4 Color; // 乗算カラー (R, G, B, A)
    float Intensity; // 輝度倍率
    float3 _pad; // 16byte アライメント用パディング
};

// t0 : インスタンスデータ（全スプライト分）
StructuredBuffer<SpriteShaderData> gInstanceData : register(t0);

struct VSInput
{
    float3 Position : POSITION; // 単位クワッド頂点座標 {0,0,0}〜{1,1,0}
    float2 TexCoord : TEXCOORD; // UV 座標 {0,0}〜{1,1}
};


struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR; // PS へ乗算カラーを渡す
    float Intensity : INTENSITY; // PS へ輝度倍率を渡す
};


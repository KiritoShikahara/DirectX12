#pragma once

#include<graphics/Color/Color.h>
#include<DirectXMath.h>

namespace graphics
{
    struct SpriteVertex
    {
        DirectX::XMFLOAT3 Position;   // 単位クワッド座標 {0,0,0}〜{1,1,0}
        DirectX::XMFLOAT2 TexCoord;   // UV 座標         {0,0}〜{1,1}
    };
    static_assert(sizeof(SpriteVertex) == 20, "SpriteVertex size mismatch");


    /// <summary>
    /// スプライト1件分のシェーダー定数データ。
    /// SpriteRenderer から StructuredBuffer 経由でシェーダーへ渡す。
    ///
    /// 重要: #pragma pack は使用しない。
    /// HLSL の StructuredBuffer 要素は暗黙的に「1メンバーが16バイト境界を跨がない」
    /// パッキングルールが適用されるため、C++ 側もそれに明示的に合わせる必要がある。
    /// (#pragma pack(1) で詰めると、HLSL 側が自動挿入するパディングと食い違い、
    ///  要素ごとにオフセットがずれて隣の要素のデータを読んでしまう)
    /// </summary>
    struct SpriteShaderData
    {
        DirectX::XMFLOAT4X4 WVP = {};                        // 64 bytes (offset   0)
        Color               Color = graphics::Color::White; // 16 bytes (offset  64)
        float               Intensity = 1.0f;                //  4 bytes (offset  80)
        float               FillAmount = 1.0f;                //  4 bytes (offset  84)
        int                 FillType = 0;                    //  4 bytes (offset  88)
        float               _pad0 = 0.0f;                     //  4 bytes (offset  92) HLSL側の16バイト境界揃えに合わせる明示パディング
        DirectX::XMFLOAT2   UVScale = { 1.0f, 1.0f };         //  8 bytes (offset  96)
        DirectX::XMFLOAT2   UVOffset = { 0.0f, 0.0f };        //  8 bytes (offset 104)
    };
    static_assert(sizeof(SpriteShaderData) == 112, "SpriteShaderData size mismatch");
    static_assert(offsetof(SpriteShaderData, UVScale) == 96, "UVScale must start on a 16-byte boundary to match HLSL packing");

    /// <summary>
    /// FBXの頂点構造体
    /// </summary>
    struct SkeletalMeshVertex
    {
        DirectX::XMFLOAT3 Position;     // 頂点位置
        DirectX::XMFLOAT2 UV;           // テクスチャ座標
        DirectX::XMFLOAT3 Normal;       // 法線
        uint32_t Bone[4];               // ボーンインデックス（0-255想定）
        float Weight[4];                // ウェイト（0.0f〜1.0f、合計1.0f）

        SkeletalMeshVertex()
        {
            Position = { 0.f, 0.f, 0.f };
            Normal = { 0.f, 0.f, 0.f };
            UV = { 0.f, 0.f };
            for (int i = 0; i < 4; i++)
            {
                Bone[i] = 0;
                Weight[i] = 0.f;
            }
        }
    };

}
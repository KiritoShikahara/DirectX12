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
    /// </summary>
#pragma pack(push, 1)
    struct SpriteShaderData
    {
        DirectX::XMFLOAT4X4 WVP = {};
        Color               Color = graphics::Color::White;
        float               Intensity = 1.0f;
        float               FillAmount = 1.0f;
        int                 FillType = 0;
        float               _pad[1] = {};
    };
#pragma pack(pop)
    static_assert(sizeof(SpriteShaderData) == 96, "SpriteShaderData size mismatch");

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
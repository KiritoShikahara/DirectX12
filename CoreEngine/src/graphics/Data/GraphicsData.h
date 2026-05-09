#pragma once

#include<graphics/Color/Color.h>
#include<DirectXMath.h>

namespace graphics
{
    struct SpriteVertex
    {
        DirectX::XMFLOAT3 Pos;   // 座標
        DirectX::XMFLOAT2 UV;    // テクスチャ座標
    };

    struct SpriteShaderData {
        DirectX::XMFLOAT4X4 WVP;    // ワールド・ビュー・プロジェクション 64
        Color Color;  // 色 16
        float Intensity; // 光度 4
        float Padding[3];           // 12byte
    };

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
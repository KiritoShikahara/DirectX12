#pragma once

#include<DirectXMath.h>
#include<cstdint>
#include<string>
#include<Utility/Export/Export.h>

namespace graphics
{
	class Texture;

    /// <summary>
    /// GPU頂点レイアウト
    /// </summary>
    struct FbxVertex
    {
        DirectX::XMFLOAT3 Position;     // offset  0  12 bytes
        DirectX::XMFLOAT2 UV;           // offset 12   8 bytes
        DirectX::XMFLOAT3 Normal;       // offset 20  12 bytes
        DirectX::XMFLOAT3 Tangent;      // offset 32  12 bytes
        int32_t           Bone[4];      // offset 44  16 bytes
        float             Weight[4];    // offset 60  16 bytes
    };
    static_assert(sizeof(FbxVertex) == 76, "FbxVertex size mismatch");

    /// <summary>
    /// GPUインスタンスデータ t0
    /// </summary>
    struct alignas(16) FbxInstanceData
    {
        // Row 0-3 : ワールド行列 (転置済み)
        DirectX::XMFLOAT4X4 World;            // 64 bytes

        // Row 4 : ベースカラー係数 + メタリック係数
        DirectX::XMFLOAT3   BaseColorFactor;  // 12
        float               MetallicFactor;   //  4

        // Row 5 : ラフネス係数 + エミッシブ係数
        float               RoughnessFactor;  //  4
        DirectX::XMFLOAT3   EmissiveFactor;   // 12

        // Row 6 : ボーン参照範囲 + テクスチャ有無フラグ [0..1]
        uint32_t            BoneOffset;       //  4  (BoneBuffer 内の先頭インデックス)
        uint32_t            BoneCount;        //  4  (0 = スキニングなし)
        uint32_t            HasAlbedo;        //  4
        uint32_t            HasNormal;        //  4

        // Row 7 : テクスチャ有無フラグ [2..5]
        uint32_t            HasMetallic;      //  4
        uint32_t            HasRoughness;     //  4
        uint32_t            HasAO;            //  4
        uint32_t            HasEmissive;      //  4
    };
    static_assert(sizeof(FbxInstanceData) == 128);
    static_assert(sizeof(FbxInstanceData) % 16 == 0);

    /// <summary>
    /// シーン共通データ
    /// </summary>
    struct alignas(16) FbxSceneData
    {
        DirectX::XMFLOAT4X4 ViewProjection;   // 64 bytes (転置済み)
        DirectX::XMFLOAT3   CameraPosition;   // 12
        float               _pad0;            //  4
        DirectX::XMFLOAT3   LightDirection;   // 12 (ワールド空間、シーン→光源方向)
        float               LightIntensity;   //  4
        DirectX::XMFLOAT3   LightColor;       // 12
        float               _pad1;            //  4
    };
    static_assert(sizeof(FbxSceneData) == 112);

    /// <summary>
    /// １セクションごとの情報（マテリアル単位の描画情報）
    /// </summary>
    struct ENGINE_API FbxSection
    {
        std::string Name;

        // PBRテクスチャ
        Texture* AlbedoTexture = nullptr;
        Texture* NormalTexture = nullptr;
        Texture* MetallicTexture = nullptr;
        Texture* RoughnessTexture = nullptr;
        Texture* AOTexture = nullptr;
        Texture* EmissiveTexture = nullptr;

        // フォールバック
        DirectX::XMFLOAT3 BaseColorFactor = { 1.f, 1.f, 1.f };
        float             MetallicFactor = 0.0f;
        float             RoughnessFactor = 0.5f;
        DirectX::XMFLOAT3 EmissiveFactor = { 0.f, 0.f, 0.f };

        // 描画範囲
        uint32_t IndexOffset = 0;
        uint32_t IndexCount = 0;
    };

    /// <summary>
    /// ボーン情報
    /// </summary>
    struct ENGINE_API FbxBoneData
    {
        std::string             Name;
        int32_t                 ParentIndex = -1;
        DirectX::XMFLOAT4X4    BindMatrix = {};
    };
}
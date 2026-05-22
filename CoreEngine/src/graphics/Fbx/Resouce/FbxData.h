#pragma once

#include<vector>
#include<memory>
#include <DirectXMath.h>
#include<string>
#include<Utility/Export/Export.h>

namespace graphics
{
    class Texture;

    // ============================================================
    //  GPU 頂点レイアウト（FbxAnalyzer バイナリと 1:1 対応）
    // ============================================================
    struct FbxVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 UV;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 Tangent;
        DirectX::XMFLOAT4 Color;
        int32_t           Bone[4];
        float             Weight[4];
    };

    // ============================================================
    //  マテリアル単位のサブメッシュ情報
    // ============================================================
    struct ENGINE_API FbxSection
    {
        std::string MaterialName;
        std::string DiffuseTexturePath;
        std::string NormalTexturePath;
        std::string MetallicTexturePath;
        std::string RoughnessTexturePath;

        DirectX::XMFLOAT4 BaseColor = { 1.f, 1.f, 1.f, 1.f };
        float             Metallic = 0.f;
        float             Roughness = 0.5f;

        uint32_t IndexOffset = 0;
        uint32_t IndexCount = 0;

        Texture* DiffuseTexture = nullptr;
        Texture* NormalTexture = nullptr;
    };

    // ============================================================
    //  ボーン情報
    // ============================================================
    struct ENGINE_API FbxBoneData
    {
        std::string           Name;
        int32_t               ParentIndex = -1;
        DirectX::XMFLOAT4X4   BindMatrix;
    };

    // ============================================================
    //  アニメーションクリップ
    // ============================================================
    struct ENGINE_API FbxAnimClip
    {
        std::string Name;
        int32_t     NumFrame = 0;
        float       StartTime = 0.f;
        float       StopTime = 0.f;
        // KeyFrames[BoneIndex][FrameIndex] = ローカル変換行列
        std::vector<std::vector<DirectX::XMFLOAT4X4>> KeyFrames;
    };

    // ============================================================
    //  GPU インスタンスデータ（StructuredBuffer t0）
    // ============================================================
    struct alignas(16) FbxInstanceData
    {
        DirectX::XMFLOAT4X4 World;       // ワールド行列（転置済み）
        DirectX::XMFLOAT4   BaseColor;
        float               Metallic;
        float               Roughness;
        float               Intensity;
        uint32_t            BoneOffset;
        uint32_t            BoneCount;
        float               _pad[3];
    };

    // ============================================================
    //  カメラ GPU データ（StructuredBuffer t3、1要素のみ使用）
    // ============================================================
    struct alignas(16) CameraShaderData
    {
        DirectX::XMFLOAT4X4 ViewProjection;  // VP 行列（転置済み）
        DirectX::XMFLOAT3   Position;        // カメラワールド座標
        float               _pad = 0.f;
    };
    static_assert(sizeof(CameraShaderData) == 80, "CameraShaderData size mismatch");
}
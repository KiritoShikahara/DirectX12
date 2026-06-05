#pragma once

#include<vector>
#include<string>
#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace graphics
{
	class Texture;

	// 頂点レイアウト
    struct ENGINE_API ModelVertex
    {
        DirectX::XMFLOAT3 Position;      
        DirectX::XMFLOAT3 Normal;        
        DirectX::XMFLOAT2 UV0;           
        DirectX::XMFLOAT2 UV1;           
        DirectX::XMFLOAT3 Tangent;       
        DirectX::XMFLOAT3 Bitangent;     
        uint8_t           BoneIndices[4];
        float             BoneWeights[4];
    };
    static_assert(sizeof(ModelVertex) == 84, "ModelVertex size mismatch");

	// マテリアルのセクション
    struct ENGINE_API ModelSection
    {
        std::string MeshName;
        std::string MaterialName;
        std::string DiffuseTexPath;
        std::string NormalTexPath;
        std::string SpecularTexPath;
        std::string EmissiveTexPath;

        DirectX::XMFLOAT4 BaseColor = { 1.f, 1.f, 1.f, 1.f };
        float             Metallic = 0.f;
        float             Roughness = 0.5f;
        uint32_t          Flags = 0;

        uint32_t IndexOffset = 0;
        uint32_t IndexCount = 0;

        Texture* DiffuseTexture = nullptr;
        Texture* NormalTexture = nullptr;
        Texture* SpecularTexture = nullptr;
    };

    // ボーンデータ
    struct ENGINE_API ModelBoneData
    {
        std::string          Name;
        int32_t              ParentIndex = -1;
        DirectX::XMFLOAT4X4  OffsetMatrix;
        DirectX::XMFLOAT4X4  LocalTransform;
    };

    // ベイク済みアニメーションフレーム
    struct ModelBakedFrame
    {
        DirectX::XMFLOAT3 Translation;
        DirectX::XMFLOAT4 Rotation;
        DirectX::XMFLOAT3 Scale;      
    };
    static_assert(sizeof(ModelBakedFrame) == 40);

    struct ModelBakedTrack
    {
        std::string                  BoneName;
        int32_t                      BoneIndex = -1;
        std::vector<ModelBakedFrame> Frames;
    };

    // スパースキー
    struct ModelPosKey { float time; DirectX::XMFLOAT3 value; };
    struct ModelRotKey { float time; DirectX::XMFLOAT4 value; };
    struct ModelScaleKey { float time; DirectX::XMFLOAT3 value; };
    static_assert(sizeof(ModelPosKey) == 16);
    static_assert(sizeof(ModelRotKey) == 20);
    static_assert(sizeof(ModelScaleKey) == 16);

    struct ModelSparseTrack
    {
        std::string                   BoneName;
        int32_t                       BoneIndex = -1;
        std::vector<ModelPosKey>      PosKeys;
        std::vector<ModelRotKey>      RotKeys;
        std::vector<ModelScaleKey>    ScaleKeys;
    };

    // アニメーションクリップ
    struct ENGINE_API ModelAnimClip
    {
        std::string Name;
        float       Duration = 0.f;
        bool        IsBaked = false;
        float       BakeFrameRate = 0.f;

        std::vector<ModelBakedTrack>  BakedTracks;   // IsBaked==true
        std::vector<ModelSparseTrack> SparseTracks;  // IsBaked==false
    };

    // GPUインスタンスデータ
    struct alignas(16) ModelInstanceData
    {
        DirectX::XMFLOAT4X4 World;       // ワールド行列
        DirectX::XMFLOAT4   BaseColor;
        float               Metallic;
        float               Roughness;
        float               Intensity;
        uint32_t            BoneOffset;  // BoneBuffer 内の先頭インデックス
        uint32_t            BoneCount;
        float               _pad[3];
    };
    static_assert(sizeof(ModelInstanceData) % 16 == 0);

    // カメラGPUデータ
    struct alignas(16) ModelCameraData
    {
        DirectX::XMFLOAT4X4 ViewProjection; // 転置済み
        DirectX::XMFLOAT3   Position;
        float               _pad = 0.f;
    };
    static_assert(sizeof(ModelCameraData) == 80);
} // graphics

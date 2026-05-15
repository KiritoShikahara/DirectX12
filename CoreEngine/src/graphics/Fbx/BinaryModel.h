#pragma once

#include <DirectXMath.h>
#include <cstdint>
#include <string>
#include <vector>

namespace graphics
{
    struct BinVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 UV;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 Tangent;
        DirectX::XMFLOAT4 Color;
        int32_t           BoneIndex[4];
        float             Weight[4];
    };
    static_assert(sizeof(BinVertex) == 92, "BinVertex size mismatch with FbxAnalyzer output");

    struct BinMaterial
    {
        std::string            Name;
        std::string            DiffuseTexture;
        std::string            NormalTexture;
        std::string            MetallicTexture;
        std::string            RoughnessTexture;
        DirectX::XMFLOAT4      BaseColor{ 1,1,1,1 };
        float                  Metallic{ 0.f };
        float                  Roughness{ 0.5f };
        uint32_t               PolygonCount{ 0 };
    };

    struct BinBone
    {
        std::string            Name;
        int32_t                ParentIndex;   // -1 = root
        DirectX::XMFLOAT4X4   BindMatrix;    // LinkMatrix.Inverse() * trans
    };

    struct BinModel
    {
        int32_t                  MeshCount = 0;
        int32_t                  PolygonCount = 0;
        int32_t                  VertexCount = 0;
        std::vector<BinVertex>   Vertices;
        std::vector<uint32_t>    Indices;
        std::vector<BinMaterial> Materials;
        std::vector<BinBone>     Bones;
    };

    struct BinAnimClip
    {
        std::string              Name;
        int32_t                  NumFrame = 0;
        float                    StartTime = 0.f;
        float                    StopTime = 0.f;
        // KeyFrames[boneIdx][frameIdx] = ÉçÅ[ÉJÉãïœä∑çsóÒ (row-major, EvaluateLocalTransform)
        std::vector<std::vector<DirectX::XMFLOAT4X4>> KeyFrames;
    };

    struct BinAnimation
    {
        std::vector<BinAnimClip> Clips;
    };
}
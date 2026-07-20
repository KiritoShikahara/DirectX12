#pragma once

#include<DirectXMath.h>
#include<cstdint>
#include<string>
#include<vector>
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

        // 色
        DirectX::XMFLOAT4 CustomColor = { 1,1,1,1 }; // 16
    };
    static_assert(sizeof(FbxInstanceData) == 144);
    static_assert(sizeof(FbxInstanceData) % 16 == 0);

    /// <summary>
    /// シーン共通データ
    /// </summary>
    struct alignas(16) FbxSceneData
    {
        DirectX::XMFLOAT4X4 ViewProjection;   // 64 bytes (転置済み)
        DirectX::XMFLOAT3   CameraPosition;   // 12
        uint32_t LightCount;    //  4 ライトの数
    };
    static_assert(sizeof(FbxSceneData) == 80);

    /// <summary>
    /// ライトデータ
    /// 全種類のライトをtypeフィールドで識別して格納。
    /// LightViewProj を追加し、Directional Light の Shadow Map 投影に使用する。
    /// Point / Spot は現状未使用 (将来拡張用)
    /// </summary>
    struct alignas(16) LightData
    {
        DirectX::XMFLOAT3 Color;        // 12
        float             Intensity;    //  4

        DirectX::XMFLOAT3 Direction;    // 12  Directional / Spot 用 (正規化済み)
        float             Range;        //  4  Point / Spot 用

        DirectX::XMFLOAT3 Position;     // 12  Point / Spot 用
        uint32_t          Type;         //  4  eLightType

        float             InnerCosine;  //  4  Spot 用 cos(InnerConeRad)
        float             OuterCosine;  //  4  Spot 用 cos(OuterConeRad)
        // Shadow Map 投影行列 (CPU側転置済み)
        // Directional Light のみ有効。Point/Spot は未使用 (将来拡張用)
        uint32_t          CastShadow;   //  4  1 = Shadow Map あり
        float             ShadowBias;   //  4  セルフシャドウ除去バイアス (推奨: 0.005)

        DirectX::XMFLOAT4X4 LightViewProj; // 64 bytes (転置済み)
    };
    static_assert(sizeof(LightData) == 128);
    static_assert(sizeof(LightData) % 16 == 0);

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
        DirectX::XMFLOAT4X4    LocalTransform = {};
    };

    /// <summary>
    /// キーフレーム1点分の変換をTRS(スケール・回転クォータニオン・平行移動)へ分解したもの。
    ///
    /// アニメーション補間はTRS空間で行う必要があるが、XMMatrixDecomposeは非常に高価で、
    /// かつ分解結果は「クリップ・ボーン・フレーム」だけで決まりエンティティには依存しない。
    /// 行列のまま持つと再生中のキャラクター1体ごとに毎フレーム同じ分解を繰り返すことになる
    /// (同じモデルの敵が100体いれば100回の重複)ため、読み込み時に一度だけ分解して保持する。
    /// </summary>
    struct FbxKeyFrameTrs
    {
        DirectX::XMFLOAT4 Scale = { 1.f, 1.f, 1.f, 0.f };       // xyzのみ使用
        DirectX::XMFLOAT4 Rotation = { 0.f, 0.f, 0.f, 1.f };    // クォータニオン
        DirectX::XMFLOAT4 Translation = { 0.f, 0.f, 0.f, 1.f }; // xyzのみ使用
    };

    /// <summary>
    /// アニメーションクリップ（.anmのエントリ）
    /// </summary>
    struct ENGINE_API FbxAnimClip
    {
        std::string Name;
        int         NumFrame = 0;
        float       FrameRate = 60.0f;
        float       Duration = 0.0f;   // NumFrame / FrameRate

        // KeyFrames[BoneIndex][FrameIndex] = ローカル変換行列 (未転置)
        std::vector<std::vector<DirectX::XMFLOAT4X4>> KeyFrames;

        // KeyFrameTrs[BoneIndex][FrameIndex] = 上記をTRS分解したもの(読み込み時に一度だけ構築)。
        // 実行時の補間はこちらだけを参照し、XMMatrixDecomposeを一切呼ばない
        std::vector<std::vector<FbxKeyFrameTrs>> KeyFrameTrs;
    };

    // カメラGPUデータ
    struct alignas(16) FbxCameraData
    {
        DirectX::XMFLOAT4X4 ViewProjection;
        DirectX::XMFLOAT3   Position;
        float               _pad = 0.f;
    };
    static_assert(sizeof(FbxCameraData) == 80);
}

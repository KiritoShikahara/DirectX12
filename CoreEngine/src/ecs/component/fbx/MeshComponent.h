#pragma once

#include<Utility/Export/Export.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <memory>
#include <vector>

namespace graphics
{
    class Mesh;
    class Skeleton;
    class AnimationClip;
}

namespace ecs
{
    /// <summary>
    /// MeshComponent
    /// </summary>
    struct ENGINE_API MeshComponent
    {
        std::shared_ptr<graphics::Mesh> mesh;

        // Diffuse テクスチャの GPU ハンドル。
        // テクスチャロード後に TextureManager 等から取得して設定する。
        // 未設定 (ptr==0) の場合はテクスチャなしで描画する。
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = {};
    };

    //  AnimationSystem が毎フレーム skinMatrices を更新する。
    //  MeshRenderer が skinMatrices を GPU へ転送して描画に使用する。
    struct ENGINE_API SkeletonComponent
    {
        std::shared_ptr<graphics::Skeleton> skeleton;

        // skinMatrices[i] = BindMatrix[i] * globalBoneTransform[i]
        std::vector<DirectX::XMFLOAT4X4> skinMatrices;
        bool dirty = true;
    };

	// AnimationComponent は AnimationSystem が毎フレーム更新する。
    struct ENGINE_API AnimationComponent
    {
        std::shared_ptr<graphics::AnimationClip> clip;
        float   currentTime = 0.f;
        float   playbackRate = 1.f;
        bool    looping = true;
        bool    playing = true;

        void Play(std::shared_ptr<graphics::AnimationClip> newClip, bool fromStart = true)
        {
            clip = std::move(newClip);
            if (fromStart) currentTime = 0.f;
            playing = true;
        }
        void Stop() { playing = false; }
        void Resume() { playing = true; }
    };
}
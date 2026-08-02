#pragma once

#include <vector>
#include <string>
#include <DirectXMath.h>
#include <Utility/Export/Export.h>

namespace graphics { class FbxResource; }

namespace ecs
{
    /// <summary>
    /// FBXアニメーションコンポーネント
    /// </summary>
    struct ENGINE_API FbxAnimComponent
    {
        /// <summary>現在のクリップインデックス</summary>
        int   CurrentClipIndex = 0;
        /// <summary>現在の再生時間</summary>
        float CurrentTime = 0.f;
        /// <summary>再生速度</summary>
        float PlaySpeed = 1.f;
        /// <summary>ループ再生するかどうか</summary>
        bool  IsLoop = true;
        /// <summary>再生中かどうか</summary>
        bool  IsPlaying = true;

        /// <summary>クロスフェード前のクリップインデックス</summary>
        int   PrevClipIndex = -1;
        /// <summary>クロスフェード前の再生時間</summary>
        float PrevTime = 0.f;
        /// <summary>ブレンド経過時間</summary>
        float BlendTime = 0.f;
        /// <summary>ブレンドにかかる時間</summary>
        float BlendDuration = 0.2f;

        /// <summary>スキニング行列配列</summary>
        std::vector<DirectX::XMFLOAT4X4> BoneMatrices;

        /// <summary>アニメーションを再生する</summary>
        void Play(int clipIndex = 0, bool loop = true);
        /// <summary>アニメーションを再生する</summary>
        void Play(const graphics::FbxResource& resource,
            const std::string& clipName, bool loop = true);

        /// <summary>クロスフェードでアニメーションを切り替える</summary>
        void CrossFade(int newClipIndex, float blendDuration = 0.2f, bool loop = true);
        /// <summary>クロスフェードでアニメーションを切り替える</summary>
        void CrossFade(const graphics::FbxResource& resource,
            const std::string& clipName,
            float blendDuration = 0.2f, bool loop = true);

        /// <summary>一時停止する</summary>
        void Pause() { IsPlaying = false; }
        /// <summary>再開する</summary>
        void Resume() { IsPlaying = true; }
        /// <summary>巻き戻す</summary>
        void Rewind() { CurrentTime = 0.f; }

        /// <summary>ブレンド係数を取得する</summary>
        float GetBlendFactor() const
        {
            return (BlendDuration > 0.f)
                ? std::min(BlendTime / BlendDuration, 1.f)
                : 1.f;
        }

        /// <summary>再生時間を進める</summary>
        void Update(float deltaTime, const graphics::FbxResource& resource);

        /// <summary>スキニング行列を計算して書き込む</summary>
        void CalcBoneMatrices(const graphics::FbxResource& resource);

    private:
        /// <summary>ローカル行列を評価する</summary>
        static void EvalLocalMats(
            const graphics::FbxResource& resource,
            int clipIndex, float time,
            std::vector<DirectX::XMMATRIX>& outLocal);

        /// <summary>スキン行列を構築する</summary>
        void BuildSkinMatrices(
            const graphics::FbxResource& resource,
            const std::vector<DirectX::XMMATRIX>& localMats);

        std::vector<DirectX::XMMATRIX> mCurrLocal;
        std::vector<DirectX::XMMATRIX> mPrevLocal;
        std::vector<DirectX::XMMATRIX> mBlendedLocal;
        std::vector<DirectX::XMMATRIX> mWorldMats;
    };
}
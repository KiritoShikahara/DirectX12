#pragma once

#include<vector>
#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace graphics
{
    class FbxResource;
}

namespace ecs
{
    struct ENGINE_API AnimationComponent
    {
        // --------------------------------------------------
        //  再生状態
        // --------------------------------------------------

        /// <summary>現在再生中のクリップインデックス</summary>
        int   CurrentClipIndex = 0;

        /// <summary>クリップ先頭からの経過時間（秒）</summary>
        float CurrentTime = 0.f;

        /// <summary>再生速度倍率（負値で逆再生）</summary>
        float PlaySpeed = 1.f;

        /// <summary>ループ再生するか</summary>
        bool  IsLoop = true;

        /// <summary>再生中か</summary>
        bool  IsPlaying = true;

        /// <summary>
        /// CalcBoneMatrices() 呼び出し後に更新されるスキニング行列配列。
        /// mBoneMatrices[i] = SkinMatrix[i]（HLSL 用に転置済み）。
        /// </summary>
        std::vector<DirectX::XMFLOAT4X4> BoneMatrices;

        /// <summary>指定クリップを再生開始する</summary>
        void Play(int clipIndex = 0, bool loop = true)
        {
            CurrentClipIndex = clipIndex;
            CurrentTime = 0.f;
            IsLoop = loop;
            IsPlaying = true;
        }

        /// <summary>現在位置で一時停止する</summary>
        void Pause() { IsPlaying = false; }

        /// <summary>一時停止を解除する</summary>
        void Resume() { IsPlaying = true; }

        /// <summary>先頭に巻き戻す（IsPlaying は変えない）</summary>
        void Rewind() { CurrentTime = 0.f; }

        /// <summary>
        /// デルタタイムで再生時刻を進める。
        /// ループ・一時停止を処理する。
        /// </summary>
        void Update(float deltaTime, const graphics::FbxResource& resource);

        /// <summary>
        /// 現在フレームのスキニング行列を BoneMatrices に書き込む。
        /// Update() の後に呼ぶこと。
        /// </summary>
        void CalcBoneMatrices(const graphics::FbxResource& resource);

    private:
        int CalcCurrentFrame(int numFrame, float startTime, float stopTime) const;
    };
}
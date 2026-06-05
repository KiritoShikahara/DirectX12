#pragma once

#include <vector>
#include <string>
#include <DirectXMath.h>
#include <Utility/Export/Export.h>
#include<graphics/Model/ModelData.h>

namespace graphics { class ModelResource; }

namespace ecs
{
    struct ENGINE_API ModelAnimComponent
    {
        // ── 現在クリップ ──────────────────────────────────────────────
        int   CurrentClipIndex = 0;
        float CurrentTime = 0.f;
        float PlaySpeed = 1.f;
        bool  IsLoop = true;
        bool  IsPlaying = true;

        // ── ブレンド ──────────────────────────────────────────────────
        int   PrevClipIndex = -1;
        float PrevTime = 0.f;
        float BlendTime = 0.f;
        float BlendDuration = 0.2f;

        // ── 出力 ─────────────────────────────────────────────────────
        std::vector<DirectX::XMFLOAT4X4> BoneMatrices;

        // ── 状態取得 ─────────────────────────────────────────────────
        bool  IsBlending()     const { return PrevClipIndex >= 0; }
        float GetBlendFactor() const
        {
            return (BlendDuration > 0.f)
                ? std::min(BlendTime / BlendDuration, 1.f) : 1.f;
        }

        // ── 再生制御 ─────────────────────────────────────────────────
        void Play(int clipIndex = 0, bool loop = true);
        void Play(const graphics::ModelResource& resource,
            const std::string& clipName, bool loop = true);
        void CrossFade(int newClipIndex, float blendDuration = 0.2f, bool loop = true);
        void CrossFade(const graphics::ModelResource& resource,
            const std::string& clipName,
            float blendDuration = 0.2f, bool loop = true);
        void Pause() { IsPlaying = false; }
        void Resume() { IsPlaying = true; }
        void Rewind() { CurrentTime = 0.f; }

        void Update(float deltaTime, const graphics::ModelResource& resource);
        void CalcBoneMatrices(const graphics::ModelResource& resource);

    private:
        // アニメーションクリップを localMats に上書きする
        static void ApplyClip(
            const graphics::ModelAnimClip& clip,
            float                               time,
            std::vector<DirectX::XMMATRIX>& localMats);

        // スパース補間ヘルパー
        static DirectX::XMVECTOR EvalPosV(const std::vector<graphics::ModelPosKey>&, float t);
        static DirectX::XMVECTOR EvalRotV(const std::vector<graphics::ModelRotKey>&, float t);
        static DirectX::XMVECTOR EvalScaleV(const std::vector<graphics::ModelScaleKey>&, float t);
    };

} // namespace ecs
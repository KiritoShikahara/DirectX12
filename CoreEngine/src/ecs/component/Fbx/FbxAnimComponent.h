#pragma once
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <Utility/Export/Export.h>

namespace graphics { class FbxResource; }

namespace ecs
{
    // ============================================================
    //  FbxAnimComponent  (ModelAnimComponent 相当)
    //
    //  アニメーション方式:
    //    FbxAnalyzer が 60fps でベイクしたローカル行列配列を使用
    //    ブレンド時は TRS 分解 → Lerp/Slerp → 再合成 で行う
    //
    //  BoneMatrices は CalcBoneMatrices() が転置済みで格納する
    //  (FbxRenderer::Submit() はそのまま BoneBuffer に積む)
    // ============================================================
    struct ENGINE_API FbxAnimComponent
    {
        // ── 再生状態 ──────────────────────────────────────────
        int   CurrentClipIndex = 0;
        float CurrentTime = 0.f;
        float PlaySpeed = 1.f;
        bool  IsLoop = true;
        bool  IsPlaying = true;

        // ── クロスフェード用 ───────────────────────────────────
        int   PrevClipIndex = -1;    // -1 = ブレンドなし
        float PrevTime = 0.f;
        float BlendTime = 0.f;
        float BlendDuration = 0.2f;

        /// <summary>
        /// CalcBoneMatrices() が生成するスキニング行列配列
        /// HLSL 用に転置済み (FbxRenderer::Submit でそのまま BoneBuffer へ積む)
        /// </summary>
        std::vector<DirectX::XMFLOAT4X4> BoneMatrices;

        // ── 再生制御 ──────────────────────────────────────────
        void Play(int clipIndex = 0, bool loop = true);
        void Play(const graphics::FbxResource& resource,
            const std::string& clipName, bool loop = true);

        void CrossFade(int newClipIndex, float blendDuration = 0.2f, bool loop = true);
        void CrossFade(const graphics::FbxResource& resource,
            const std::string& clipName,
            float blendDuration = 0.2f, bool loop = true);

        void Pause() { IsPlaying = false; }
        void Resume() { IsPlaying = true; }
        void Rewind() { CurrentTime = 0.f; }

        float GetBlendFactor() const
        {
            return (BlendDuration > 0.f)
                ? std::min(BlendTime / BlendDuration, 1.f)
                : 1.f;
        }

        /// <summary>再生時間を deltaTime だけ進める (ループ・ブレンド時間の管理も行う)</summary>
        void Update(float deltaTime, const graphics::FbxResource& resource);

        /// <summary>
        /// 現フレームのスキニング行列を BoneMatrices に書き込む
        /// Update() の後に呼ぶこと
        /// </summary>
        void CalcBoneMatrices(const graphics::FbxResource& resource);

    private:
        // 指定クリップ/時刻の全ボーン ローカル行列を返す
        static void EvalLocalMats(
            const graphics::FbxResource& resource,
            int clipIndex, float time,
            std::vector<DirectX::XMMATRIX>& outLocal);

        // ローカル行列配列からスキン行列を構築して BoneMatrices へ書き込む
        void BuildSkinMatrices(
            const graphics::FbxResource& resource,
            const std::vector<DirectX::XMMATRIX>& localMats);
    };

} // namespace ecs
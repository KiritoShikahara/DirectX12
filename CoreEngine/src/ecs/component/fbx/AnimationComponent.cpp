#include"pch.h"
#include "AnimationComponent.h"
#include<graphics/Fbx/Resouce/FbxSource.h>

namespace ecs
{
	void AnimationComponent::Update(float deltaTime, const graphics::FbxResource& resource)
	{
        if (!IsPlaying || !resource.HasAnimation()) return;

        const auto& clips = resource.GetAnimClips();
        if (CurrentClipIndex < 0 || CurrentClipIndex >= static_cast<int>(clips.size())) return;

        const auto& clip = clips[CurrentClipIndex];
        const float duration = clip.StopTime - clip.StartTime;
        if (duration <= 0.f) return;

        CurrentTime += deltaTime * PlaySpeed;

        // ループ・終端処理
        if (CurrentTime >= duration)
        {
            if (IsLoop)
            {
                CurrentTime = std::fmod(CurrentTime, duration);
            }
            else
            {
                CurrentTime = duration;
                IsPlaying = false;
            }
        }
        else if (CurrentTime < 0.f)
        {
            // 逆再生時
            if (IsLoop) CurrentTime = duration + std::fmod(CurrentTime, duration);
            else { CurrentTime = 0.f; IsPlaying = false; }
        }
    }

    /// <summary>
    /// 現在フレームのスキニング行列を BoneMatrices に書き込む。
    /// Update() の後に呼ぶこと。
    /// </summary>
    void AnimationComponent::CalcBoneMatrices(const graphics::FbxResource& resource)
    {
        if (!resource.HasSkinning()) return;

        const auto& clips = resource.GetAnimClips();
        const auto& bones = resource.GetBones();
        const int boneCount = resource.GetBoneCount();

        BoneMatrices.resize(boneCount);

        // アニメーションなしはバインドポーズをそのまま使う（恒等行列で十分だが互換性のため）
        if (!resource.HasAnimation() || clips.empty())
        {
            using namespace DirectX;
            for (int i = 0; i < boneCount; ++i)
            {
                XMStoreFloat4x4(&BoneMatrices[i],
                    XMMatrixTranspose(XMLoadFloat4x4(&bones[i].BindMatrix)));
            }
            return;
        }

        if (CurrentClipIndex < 0 || CurrentClipIndex >= static_cast<int>(clips.size())) return;
        const auto& clip = clips[CurrentClipIndex];
        const int   frame = CalcCurrentFrame(clip.NumFrame, clip.StartTime, clip.StopTime);

        using namespace DirectX;

        // グローバル行列を親→子の順で計算
        // ※ FbxAnalyzer::InitializeBone() がルートから順番にボーンを格納しているため
        //    インデックス順に計算すれば親が先に確定している
        std::vector<XMMATRIX> globalMats(boneCount, XMMatrixIdentity());

        for (int i = 0; i < boneCount; ++i)
        {
            XMMATRIX localMat = XMMatrixIdentity();

            // クリップにこのボーンのキーが存在するか確認
            if (i < static_cast<int>(clip.KeyFrames.size()) &&
                frame < static_cast<int>(clip.KeyFrames[i].size()))
            {
                localMat = XMLoadFloat4x4(&clip.KeyFrames[i][frame]);
            }

            const int parentIdx = bones[i].ParentIndex;
            if (parentIdx < 0)
            {
                globalMats[i] = localMat;
            }
            else
            {
                // グローバル = ローカル × 親グローバル（行優先行列の場合の乗算順）
                globalMats[i] = localMat * globalMats[parentIdx];
            }

            // スキニング行列 = BindMatrix × GlobalMatrix
            // HLSL へ転送するため転置する
            const XMMATRIX bindMat = XMLoadFloat4x4(&bones[i].BindMatrix);
            XMStoreFloat4x4(
                &BoneMatrices[i],
                XMMatrixTranspose(bindMat * globalMats[i]));
        }
    }

    /// <summary>
    /// 今のフレームの計算
    /// </summary>
    /// <param name="numFrame"></param>
    /// <param name="startTime"></param>
    /// <param name="stopTime"></param>
    /// <returns></returns>
    int AnimationComponent::CalcCurrentFrame(int numFrame, float startTime, float stopTime) const
    {
        const float duration = stopTime - startTime;
        if (duration <= 0.f || numFrame <= 0) return 0;

        const float t = CurrentTime / duration;
        const int frame = static_cast<int>(t * static_cast<float>(numFrame));
        return std::clamp(frame, 0, numFrame - 1);
    }
}


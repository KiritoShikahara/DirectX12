#include "pch.h"
#include "FbxAnimComponent.h"
#include <graphics/FBX/Resource/FbxResource.h>
#include <graphics/FBX/Data/FbxData.h>
#include <algorithm>
#include <cmath>

namespace ecs
{
    using namespace DirectX;

    // 再生を開始する
    void FbxAnimComponent::Play(int clipIndex, bool loop)
    {
        CurrentClipIndex = clipIndex;
        CurrentTime = 0.f;
        IsLoop = loop;
        IsPlaying = true;
        PrevClipIndex = -1;
        BlendTime = 0.f;
    }

    // 指定したクリップ名で再生を開始する
    void FbxAnimComponent::Play(const graphics::FbxResource& resource,
        const std::string& clipName, bool loop)
    {
        const int idx = resource.FindClipIndex(clipName);
        if (idx < 0)
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                std::format("FbxAnimComponent::Play: clip '{}' not found.", clipName));
            return;
        }
        Play(idx, loop);
    }

    // クロスフェードでアニメーションを切り替える
    void FbxAnimComponent::CrossFade(int newClipIndex, float blendDuration, bool loop)
    {
        if (newClipIndex == CurrentClipIndex) return;
        PrevClipIndex = CurrentClipIndex;
        PrevTime = CurrentTime;
        BlendTime = 0.f;
        BlendDuration = blendDuration;
        CurrentClipIndex = newClipIndex;
        CurrentTime = 0.f;
        IsLoop = loop;
        IsPlaying = true;
    }

    // 指定したクリップ名でクロスフェードを行う
    void FbxAnimComponent::CrossFade(const graphics::FbxResource& resource,
        const std::string& clipName,
        float blendDuration, bool loop)
    {
        const int idx = resource.FindClipIndex(clipName);
        if (idx < 0)
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                std::format("FbxAnimComponent::CrossFade: clip '{}' not found.", clipName));
            return;
        }
        CrossFade(idx, blendDuration, loop);
    }

    // 再生時間を進める
    void FbxAnimComponent::Update(float deltaTime, const graphics::FbxResource& resource)
    {
        if (!IsPlaying || !resource.HasAnimation()) return;

        const auto& clips = resource.GetAnimClips();
        if (CurrentClipIndex < 0 || CurrentClipIndex >= (int)clips.size()) return;

        // ブレンド時間を進める
        if (PrevClipIndex >= 0)
        {
            BlendTime += deltaTime;

            if (PrevClipIndex < (int)clips.size())
            {
                const float prevDur = clips[PrevClipIndex].Duration;
                if (prevDur > 0.f)
                {
                    PrevTime += deltaTime * PlaySpeed;
                    if (PrevTime >= prevDur)
                        PrevTime = std::fmod(PrevTime, prevDur);
                }
            }

            if (BlendTime >= BlendDuration)
            {
                PrevClipIndex = -1;
                BlendTime = 0.f;
            }
        }

        // 現在クリップを進める
        const float duration = clips[CurrentClipIndex].Duration;
        if (duration <= 0.f) return;

        CurrentTime += deltaTime * PlaySpeed;

        if (CurrentTime >= duration)
        {
            if (IsLoop) CurrentTime = std::fmod(CurrentTime, duration);
            else { CurrentTime = duration; IsPlaying = false; }
        }
        else if (CurrentTime < 0.f)
        {
            if (IsLoop) CurrentTime = duration + std::fmod(CurrentTime, duration);
            else { CurrentTime = 0.f; IsPlaying = false; }
        }
    }

    // ローカル行列を評価する
    void FbxAnimComponent::EvalLocalMats(
        const graphics::FbxResource& resource,
        int clipIndex, float time,
        std::vector<XMMATRIX>& outLocal)
    {
        const auto& bones = resource.GetBones();
        const int   boneCount = (int)bones.size();
        outLocal.resize(boneCount);

        // レストポーズで初期化
        for (int i = 0; i < boneCount; ++i)
            outLocal[i] = XMLoadFloat4x4(&bones[i].LocalTransform);

        const auto& clips = resource.GetAnimClips();
        if (clipIndex < 0 || clipIndex >= (int)clips.size()) return;

        const auto& clip = clips[clipIndex];
        if (clip.NumFrame <= 0) return;

        // 補間パラメータを計算
        const float frameFull = time * clip.FrameRate;
        const int   frame0 = static_cast<int>(frameFull) % clip.NumFrame;
        const int   frame1 = (frame0 + 1) % clip.NumFrame;
        const float t = frameFull - static_cast<float>(static_cast<int>(frameFull));

        const int activeBones = (int)clip.KeyFrameTrs.size();
        for (int i = 0; i < boneCount && i < activeBones; ++i)
        {
            const auto& track = clip.KeyFrameTrs[i];
            if (track.empty()) continue;

            const int f0 = std::min(frame0, (int)track.size() - 1);
            const int f1 = std::min(frame1, (int)track.size() - 1);

            const auto& k0 = track[f0];
            const auto& k1 = track[f1];

            // キーフレーム間を補間して合成
            const XMVECTOR s = XMVectorLerp(XMLoadFloat4(&k0.Scale), XMLoadFloat4(&k1.Scale), t);
            const XMVECTOR p = XMVectorLerp(XMLoadFloat4(&k0.Translation), XMLoadFloat4(&k1.Translation), t);
            const XMVECTOR r = XMQuaternionSlerp(XMLoadFloat4(&k0.Rotation), XMLoadFloat4(&k1.Rotation), t);

            outLocal[i] = XMMatrixScalingFromVector(s)
                * XMMatrixRotationQuaternion(r)
                * XMMatrixTranslationFromVector(p);
        }
    }

    // スキン行列を構築する
    void FbxAnimComponent::BuildSkinMatrices(
        const graphics::FbxResource& resource,
        const std::vector<XMMATRIX>& localMats)
    {
        const auto& bones = resource.GetBones();
        const int   boneCount = (int)bones.size();
        BoneMatrices.resize(boneCount);

        mWorldMats.resize(boneCount);

        // ワールド行列を計算
        for (int i = 0; i < boneCount; ++i)
        {
            const int parent = bones[i].ParentIndex;
            if (parent >= 0 && parent < boneCount)
                mWorldMats[i] = localMats[i] * mWorldMats[parent];
            else
                mWorldMats[i] = localMats[i];
        }

        // スキン行列を計算して転置保存
        for (int i = 0; i < boneCount; ++i)
        {
            XMMATRIX bind = XMLoadFloat4x4(&bones[i].BindMatrix);
            XMMATRIX skin = bind * mWorldMats[i];
            XMStoreFloat4x4(&BoneMatrices[i], XMMatrixTranspose(skin));
        }
    }

    // ボーン行列を計算する
    void FbxAnimComponent::CalcBoneMatrices(const graphics::FbxResource& resource)
    {
        if (!resource.HasSkinning()) return;

        EvalLocalMats(resource, CurrentClipIndex, CurrentTime, mCurrLocal);

        // クロスフェード中の処理
        if (PrevClipIndex >= 0 && BlendDuration > 0.f)
        {
            EvalLocalMats(resource, PrevClipIndex, PrevTime, mPrevLocal);

            const float alpha = GetBlendFactor();
            const int   boneCount = (int)mCurrLocal.size();
            mBlendedLocal.resize(boneCount);

            for (int i = 0; i < boneCount; ++i)
            {
                XMVECTOR sc, rc, pc, sp, rp, pp;
                XMMatrixDecompose(&sc, &rc, &pc, mCurrLocal[i]);
                XMMatrixDecompose(&sp, &rp, &pp, mPrevLocal[i]);

                const XMVECTOR s = XMVectorLerp(sp, sc, alpha);
                const XMVECTOR p = XMVectorLerp(pp, pc, alpha);
                const XMVECTOR r = XMQuaternionSlerp(rp, rc, alpha);

                mBlendedLocal[i] = XMMatrixScalingFromVector(s)
                    * XMMatrixRotationQuaternion(r)
                    * XMMatrixTranslationFromVector(p);
            }

            BuildSkinMatrices(resource, mBlendedLocal);
        }
        else
        {
            BuildSkinMatrices(resource, mCurrLocal);
        }
    }
}
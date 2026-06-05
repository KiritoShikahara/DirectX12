#include "pch.h"
#include "ModelAnimComponent.h"
#include<graphics/Model/Resouce/ModelResouce.h>
#include <algorithm>
#include <cmath>

namespace ecs
{
    using namespace DirectX;

    // ============================================================
    //  Play / CrossFade
    // ============================================================
    void ModelAnimComponent::Play(int clipIndex, bool loop)
    {
        CurrentClipIndex = clipIndex;
        CurrentTime = 0.f;
        IsLoop = loop;
        IsPlaying = true;
        PrevClipIndex = -1;
        BlendTime = 0.f;
    }

    void ModelAnimComponent::Play(const graphics::ModelResource& resource,
        const std::string& clipName, bool loop)
    {
        const int idx = resource.FindClipIndex(clipName);
        if (idx < 0)
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                std::format("ModelAnimComponent::Play: clip '{}' not found.", clipName));
            return;
        }
        Play(idx, loop);
    }

    void ModelAnimComponent::CrossFade(int newClipIndex, float blendDuration, bool loop)
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

    void ModelAnimComponent::CrossFade(const graphics::ModelResource& resource,
        const std::string& clipName,
        float blendDuration, bool loop)
    {
        const int idx = resource.FindClipIndex(clipName);
        if (idx < 0)
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                std::format("ModelAnimComponent::CrossFade: clip '{}' not found.", clipName));
            return;
        }
        CrossFade(idx, blendDuration, loop);
    }

    // ============================================================
    //  Update
    // ============================================================
    void ModelAnimComponent::Update(float deltaTime, const graphics::ModelResource& resource)
    {
        if (!IsPlaying || !resource.HasAnimation()) return;

        const auto& clips = resource.GetAnimClips();
        if (CurrentClipIndex < 0 || CurrentClipIndex >= (int)clips.size()) return;

        // ブレンド時間の進行
        if (PrevClipIndex >= 0)
        {
            BlendTime += deltaTime;
            if (PrevClipIndex < (int)clips.size())
            {
                const float prevDur = clips[PrevClipIndex].Duration;
                if (prevDur > 0.f)
                {
                    PrevTime += deltaTime * PlaySpeed;
                    if (PrevTime >= prevDur) PrevTime = std::fmod(PrevTime, prevDur);
                }
            }
            if (BlendTime >= BlendDuration) { PrevClipIndex = -1; BlendTime = 0.f; }
        }

        const float duration = clips[CurrentClipIndex].Duration;
        if (duration <= 0.f) return;

        CurrentTime += deltaTime * PlaySpeed;

        if (CurrentTime >= duration)
        {
            if (IsLoop)  CurrentTime = std::fmod(CurrentTime, duration);
            else { CurrentTime = duration; IsPlaying = false; }
        }
        else if (CurrentTime < 0.f)
        {
            if (IsLoop)  CurrentTime = duration + std::fmod(CurrentTime, duration);
            else { CurrentTime = 0.f; IsPlaying = false; }
        }
    }

    // ============================================================
     //  CalcBoneMatrices
     // ============================================================
    void ModelAnimComponent::CalcBoneMatrices(const graphics::ModelResource& resource)
    {
        if (!resource.HasSkinning()) return;

        const auto& bones = resource.GetBones();
        const int   boneCount = (int)bones.size();
        BoneMatrices.resize(boneCount);

        // ── Step1: LocalTransform をロードし、正しい行優先に転置して初期化 ─────────
        std::vector<XMMATRIX> localMats(boneCount);
        for (int i = 0; i < boneCount; ++i)
        {
            localMats[i] = XMMatrixTranspose(XMLoadFloat4x4(&bones[i].LocalTransform));
        }

        // ── Step2: 現在クリップを上書き（★統合された ApplyClip を通常通り呼び出します） ───
        const auto& clips = resource.GetAnimClips();
        if (resource.HasAnimation() &&
            CurrentClipIndex >= 0 &&
            CurrentClipIndex < (int)clips.size())
        {
            ApplyClip(clips[CurrentClipIndex], CurrentTime, localMats);
        }

        // ── Step3: ブレンド（★修正後の ApplyClip が適用され、ブレンド時も崩壊を防ぎます） ───
        if (PrevClipIndex >= 0 &&
            PrevClipIndex < (int)clips.size() &&
            BlendDuration > 0.f)
        {
            const float alpha = GetBlendFactor();   // 0(前) → 1(現在)

            // 前クリップの localMats を計算 (初期ポーズをロードして同様に転置)
            std::vector<XMMATRIX> prevMats(boneCount);
            for (int i = 0; i < boneCount; ++i)
                prevMats[i] = XMMatrixTranspose(XMLoadFloat4x4(&bones[i].LocalTransform));

            ApplyClip(clips[PrevClipIndex], PrevTime, prevMats);

            for (int i = 0; i < boneCount; ++i)
            {
                XMVECTOR scaleA, rotA, transA;
                XMVECTOR scaleB, rotB, transB;

                if (XMMatrixDecompose(&scaleA, &rotA, &transA, prevMats[i]) &&
                    XMMatrixDecompose(&scaleB, &rotB, &transB, localMats[i]))
                {
                    XMVECTOR blendedScale = XMVectorLerp(scaleA, scaleB, alpha);
                    XMVECTOR blendedRot = XMQuaternionNormalize(XMQuaternionSlerp(rotA, rotB, alpha));
                    XMVECTOR blendedTrans = XMVectorLerp(transA, transB, alpha);

                    localMats[i] = XMMatrixAffineTransformation(
                        blendedScale,
                        XMVectorZero(),
                        blendedRot,
                        blendedTrans);
                }
                else
                {
                    localMats[i].r[0] = XMVectorLerp(prevMats[i].r[0], localMats[i].r[0], alpha);
                    localMats[i].r[1] = XMVectorLerp(prevMats[i].r[1], localMats[i].r[1], alpha);
                    localMats[i].r[2] = XMVectorLerp(prevMats[i].r[2], localMats[i].r[2], alpha);
                    localMats[i].r[3] = XMVectorLerp(prevMats[i].r[3], localMats[i].r[3], alpha);
                }
            }
        }

        // ── Step4: グローバル行列 ──────────────────────────────────────────────
        std::vector<XMMATRIX> globalMats(boneCount, XMMatrixIdentity());
        for (int i = 0; i < boneCount; ++i)
        {
            const int parent = bones[i].ParentIndex;
            globalMats[i] = (parent < 0)
                ? localMats[i]
                : localMats[i] * globalMats[parent];   // Child * Parent
        }

        // ── Step5: スキン行列 ──────────────────────────────────────────────────
        for (int i = 0; i < boneCount; ++i)
        {
            const XMMATRIX offset = XMMatrixTranspose(XMLoadFloat4x4(&bones[i].OffsetMatrix));
            XMMATRIX skinMat = offset * globalMats[i];
            XMStoreFloat4x4(&BoneMatrices[i], XMMatrixTranspose(skinMat));
        }
    }

    // ============================================================
    //  ApplyClip : レストポーズの骨格構造(Pre-Rotation等)を維持したまま上書き
    // ============================================================
    void ModelAnimComponent::ApplyClip(
        const graphics::ModelAnimClip& clip,
        float                           time,
        std::vector<XMMATRIX>& localMats)
    {
        if (clip.IsBaked)
        {
            int maxFrame = 0;
            if (!clip.BakedTracks.empty() && !clip.BakedTracks[0].Frames.empty())
                maxFrame = (int)clip.BakedTracks[0].Frames.size() - 1;

            const int frameIdx =
                std::clamp((int)(time * clip.BakeFrameRate), 0, std::max(0, maxFrame));

            for (const auto& track : clip.BakedTracks)
            {
                if (track.BoneIndex < 0 || track.BoneIndex >= (int)localMats.size()) continue;
                if (frameIdx >= (int)track.Frames.size()) continue;

                const auto& f = track.Frames[frameIdx];

                // 1. すでにロードされている正しい初期姿勢(localMatsに入っているもの)を分解します
                XMMATRIX restMat = localMats[track.BoneIndex];
                XMVECTOR restScale, restRot, restTrans;
                XMMatrixDecompose(&restScale, &restRot, &restTrans, restMat);

                // 2. 初期回転に対して、アニメーションの回転を相対合成します
                XMVECTOR animRot = XMLoadFloat4(&f.Rotation);
                XMVECTOR finalRot = XMQuaternionNormalize(XMQuaternionMultiply(animRot, restRot));

                // 3. スケールと位置は初期姿勢を維持しつつ、合成された回転で上書きします
                localMats[track.BoneIndex] = XMMatrixAffineTransformation(
                    restScale,
                    XMVectorZero(),
                    finalRot,
                    restTrans
                );
            }
        }
        else
        {
            for (const auto& track : clip.SparseTracks)
            {
                if (track.BoneIndex < 0 || track.BoneIndex >= (int)localMats.size()) continue;

                XMMATRIX restMat = localMats[track.BoneIndex];
                XMVECTOR restScale, restRot, restTrans;
                XMMatrixDecompose(&restScale, &restRot, &restTrans, restMat);

                XMVECTOR animRot = EvalRotV(track.RotKeys, time);
                XMVECTOR finalRot = XMQuaternionNormalize(XMQuaternionMultiply(animRot, restRot));

                localMats[track.BoneIndex] = XMMatrixAffineTransformation(
                    restScale,
                    XMVectorZero(),
                    finalRot,
                    restTrans
                );
            }
        }
    }

    // ============================================================
    //  スパース補間ヘルパー
    // ============================================================
    XMVECTOR ModelAnimComponent::EvalPosV(
        const std::vector<graphics::ModelPosKey>& keys, float t)
    {
        if (keys.empty()) return XMVectorZero();
        if (keys.size() == 1) return XMLoadFloat3(&keys.front().value);
        if (t <= keys.front().time) return XMLoadFloat3(&keys.front().value);
        if (t >= keys.back().time)  return XMLoadFloat3(&keys.back().value);
        auto it = std::lower_bound(keys.begin(), keys.end(), t,
            [](const graphics::ModelPosKey& k, float time) { return k.time < time; });
        const auto& hi = *it; const auto& lo = *(it - 1);
        return XMVectorLerp(XMLoadFloat3(&lo.value), XMLoadFloat3(&hi.value),
            (t - lo.time) / (hi.time - lo.time));
    }

    XMVECTOR ModelAnimComponent::EvalRotV(
        const std::vector<graphics::ModelRotKey>& keys, float t)
    {
        if (keys.empty()) return XMQuaternionIdentity();
        if (keys.size() == 1) return XMQuaternionNormalize(XMLoadFloat4(&keys.front().value));
        if (t <= keys.front().time) return XMQuaternionNormalize(XMLoadFloat4(&keys.front().value));
        if (t >= keys.back().time)  return XMQuaternionNormalize(XMLoadFloat4(&keys.back().value));
        auto it = std::lower_bound(keys.begin(), keys.end(), t,
            [](const graphics::ModelRotKey& k, float time) { return k.time < time; });
        const auto& hi = *it; const auto& lo = *(it - 1);

        XMVECTOR q0 = XMLoadFloat4(&lo.value);
        XMVECTOR q1 = XMLoadFloat4(&hi.value);
        float factor = (t - lo.time) / (hi.time - lo.time);

        return XMQuaternionNormalize(XMQuaternionSlerp(q0, q1, factor));
    }

    XMVECTOR ModelAnimComponent::EvalScaleV(
        const std::vector<graphics::ModelScaleKey>& keys, float t)
    {
        if (keys.empty()) return XMVectorSplatOne();
        if (keys.size() == 1) return XMLoadFloat3(&keys.front().value);
        if (t <= keys.front().time) return XMLoadFloat3(&keys.front().value);
        if (t >= keys.back().time)  return XMLoadFloat3(&keys.back().value);
        auto it = std::lower_bound(keys.begin(), keys.end(), t,
            [](const graphics::ModelScaleKey& k, float time) { return k.time < time; });
        const auto& hi = *it; const auto& lo = *(it - 1);
        return XMVectorLerp(XMLoadFloat3(&lo.value), XMLoadFloat3(&hi.value),
            (t - lo.time) / (hi.time - lo.time));
    }

} // namespace ecs
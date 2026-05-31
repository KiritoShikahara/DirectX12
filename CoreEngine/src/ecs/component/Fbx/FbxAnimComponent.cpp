#include "pch.h"
#include "FbxAnimComponent.h"

#include <graphics/FBX/Resource/FbxResource.h>
#include <graphics/FBX/Data/FbxData.h>
#include <algorithm>
#include <cmath>

namespace ecs
{
    using namespace DirectX;

    // ============================================================
    //  Play / CrossFade
    // ============================================================
    void FbxAnimComponent::Play(int clipIndex, bool loop)
    {
        CurrentClipIndex = clipIndex;
        CurrentTime = 0.f;
        IsLoop = loop;
        IsPlaying = true;
        PrevClipIndex = -1;
        BlendTime = 0.f;
    }

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

    // ============================================================
    //  Update  (再生時間の進行)
    // ============================================================
    void FbxAnimComponent::Update(float deltaTime, const graphics::FbxResource& resource)
    {
        if (!IsPlaying || !resource.HasAnimation()) return;

        const auto& clips = resource.GetAnimClips();
        if (CurrentClipIndex < 0 || CurrentClipIndex >= (int)clips.size()) return;

        // ── ブレンド時間を進める ─────────────────────────────────
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

        // ── 現在クリップを進める ─────────────────────────────────
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

    // ============================================================
    //  EvalLocalMats  (内部ヘルパー)
    //  指定クリップ/時刻の全ボーン ローカル行列を返す
    //  フレーム間を TRS 分解 + Lerp/Slerp で補間する
    // ============================================================
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

        // 補間パラメータ (60fps ベイク済み)
        const float frameFull = time * clip.FrameRate;
        const int   frame0 = static_cast<int>(frameFull) % clip.NumFrame;
        const int   frame1 = (frame0 + 1) % clip.NumFrame;
        const float t = frameFull - static_cast<float>(static_cast<int>(frameFull));

        const int activeBones = (int)clip.KeyFrames.size();
        for (int i = 0; i < boneCount && i < activeBones; ++i)
        {
            const auto& track = clip.KeyFrames[i];
            if (track.empty()) continue;

            const int f0 = std::min(frame0, (int)track.size() - 1);
            const int f1 = std::min(frame1, (int)track.size() - 1);

            XMMATRIX m0 = XMLoadFloat4x4(&track[f0]);
            XMMATRIX m1 = XMLoadFloat4x4(&track[f1]);

            // TRS 分解 → 補間 → 再合成
            XMVECTOR s0, r0, p0, s1, r1, p1;
            XMMatrixDecompose(&s0, &r0, &p0, m0);
            XMMatrixDecompose(&s1, &r1, &p1, m1);

            const XMVECTOR s = XMVectorLerp(s0, s1, t);
            const XMVECTOR p = XMVectorLerp(p0, p1, t);
            const XMVECTOR r = XMQuaternionSlerp(r0, r1, t);

            outLocal[i] = XMMatrixScalingFromVector(s)
                * XMMatrixRotationQuaternion(r)
                * XMMatrixTranslationFromVector(p);
        }
    }

    // ============================================================
    //  BuildSkinMatrices  (内部ヘルパー)
    //  ローカル行列 → 親子階層 → BindMatrix → 転置して BoneMatrices へ
    // ============================================================
    void FbxAnimComponent::BuildSkinMatrices(
        const graphics::FbxResource& resource,
        const std::vector<XMMATRIX>& localMats)
    {
        const auto& bones = resource.GetBones();
        const int   boneCount = (int)bones.size();
        BoneMatrices.resize(boneCount);

        std::vector<XMMATRIX> worldMats(boneCount);

        // 親から子の順 (FBX はインデックス順に親が先)
        for (int i = 0; i < boneCount; ++i)
        {
            const int parent = bones[i].ParentIndex;
            if (parent >= 0 && parent < boneCount)
                worldMats[i] = localMats[i] * worldMats[parent];
            else
                worldMats[i] = localMats[i];
        }

        // スキン行列 = BindMatrix * WorldMatrix (転置済みで格納)
        for (int i = 0; i < boneCount; ++i)
        {
            XMMATRIX bind = XMLoadFloat4x4(&bones[i].BindMatrix);
            XMMATRIX skin = bind * worldMats[i];
            XMStoreFloat4x4(&BoneMatrices[i], XMMatrixTranspose(skin));
        }
    }

    // ============================================================
    //  CalcBoneMatrices  (メイン API)
    // ============================================================
    void FbxAnimComponent::CalcBoneMatrices(const graphics::FbxResource& resource)
    {
        if (!resource.HasSkinning()) return;

        std::vector<XMMATRIX> currLocal;
        EvalLocalMats(resource, CurrentClipIndex, CurrentTime, currLocal);

        if (PrevClipIndex >= 0 && BlendDuration > 0.f)
        {
            // ── クロスフェード: TRS 空間でブレンドして BuildSkinMatrices ──
            std::vector<XMMATRIX> prevLocal;
            EvalLocalMats(resource, PrevClipIndex, PrevTime, prevLocal);

            const float alpha = GetBlendFactor();   // 0.0(前) → 1.0(現在)
            const int   boneCount = (int)currLocal.size();
            std::vector<XMMATRIX> blended(boneCount);

            for (int i = 0; i < boneCount; ++i)
            {
                XMVECTOR sc, rc, pc, sp, rp, pp;
                XMMatrixDecompose(&sc, &rc, &pc, currLocal[i]);
                XMMatrixDecompose(&sp, &rp, &pp, prevLocal[i]);

                const XMVECTOR s = XMVectorLerp(sp, sc, alpha);
                const XMVECTOR p = XMVectorLerp(pp, pc, alpha);
                const XMVECTOR r = XMQuaternionSlerp(rp, rc, alpha);

                blended[i] = XMMatrixScalingFromVector(s)
                    * XMMatrixRotationQuaternion(r)
                    * XMMatrixTranslationFromVector(p);
            }

            BuildSkinMatrices(resource, blended);
        }
        else
        {
            BuildSkinMatrices(resource, currLocal);
        }
    }

} // namespace ecs
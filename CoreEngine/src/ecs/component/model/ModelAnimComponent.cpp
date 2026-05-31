#include"pch.h"
#include "ModelAnimComponent.h"
#include<graphics/Model/Resouce/ModelResouce.h>
#include<graphics/Model/ModelData.h>

namespace ecs
{
    using namespace DirectX;

	void ModelAnimComponent::Play(int clipIndex, bool loop)
	{
        CurrentClipIndex = clipIndex;
        CurrentTime = 0.f;
        IsLoop = loop;
        IsPlaying = true;
        // ブレンドをリセット
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
                std::format("ModelAnimComponent::Play: Clip not found: '{}'", clipName));
            return;
        }
        Play(idx, loop);
    }

    // ブレンド切り替え
    void ModelAnimComponent::CrossFade(int newClipIndex, float blendDuration, bool loop)
    {
        if (newClipIndex == CurrentClipIndex) return; // 同クリップなら何もしない

        // 現在の状態をブレンド元として退避
        PrevClipIndex = CurrentClipIndex;
        PrevTime = CurrentTime;
        BlendTime = 0.f;
        BlendDuration = blendDuration;

        // 新クリップへ切り替え
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
                std::format("ModelAnimComponent::CrossFade: Clip not found: '{}'", clipName));
            return;
        }
        CrossFade(idx, blendDuration, loop);
    }

    // 再生時間の更新
    void ModelAnimComponent::Update(float deltaTime, const graphics::ModelResource& resource)
    {
        if (!IsPlaying || !resource.HasAnimation()) return;

        const auto& clips = resource.GetAnimClips();
        if (CurrentClipIndex < 0 || CurrentClipIndex >= static_cast<int>(clips.size())) return;

        // ── ブレンド時間を進める ─────────────────────────────────
        if (PrevClipIndex >= 0)
        {
            BlendTime += deltaTime;

            // 前クリップも独立して再生を続ける (途切れなくフェードアウト)
            if (PrevClipIndex < static_cast<int>(clips.size()))
            {
                const float prevDur = clips[PrevClipIndex].Duration;
                if (prevDur > 0.f)
                {
                    PrevTime += deltaTime * PlaySpeed;
                    if (PrevTime >= prevDur)
                        PrevTime = std::fmod(PrevTime, prevDur);
                }
            }

            // ブレンド完了
            if (BlendTime >= BlendDuration)
            {
                PrevClipIndex = -1;
                BlendTime = 0.f;
            }
        }

        // ── 現在クリップの時間を進める ──────────────────────────
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

    // ブレンド＋スキン行列計算
    void ModelAnimComponent::CalcBoneMatrices(const graphics::ModelResource& resource)
    {
        if (!resource.HasSkinning()) return;

        // ── 現在クリップの TRS を計算 ────────────────────────────
        std::vector<BoneTRS> currTRS;
        CalcLocalTRS(resource, CurrentClipIndex, CurrentTime, currTRS);

        if (PrevClipIndex >= 0 && BlendDuration > 0.f)
        {
            // ── ブレンド: TRS 空間で補間 ─────────────────────────
            //   T/S → LERP、R → SLERP
            //   行列を直接補間すると skew が発生するため TRS で行う
            std::vector<BoneTRS> prevTRS;
            CalcLocalTRS(resource, PrevClipIndex, PrevTime, prevTRS);

            const float alpha = GetBlendFactor();  // 0(前) → 1(現在)
            const int   boneCount = resource.GetBoneCount();
            std::vector<BoneTRS> blended(boneCount);

            for (int i = 0; i < boneCount; ++i)
            {
                blended[i].T = XMVectorLerp(prevTRS[i].T, currTRS[i].T, alpha);
                blended[i].R = XMQuaternionSlerp(prevTRS[i].R, currTRS[i].R, alpha);
                blended[i].S = XMVectorLerp(prevTRS[i].S, currTRS[i].S, alpha);
            }

            BuildSkinMatrices(resource, blended, BoneMatrices);
        }
        else
        {
            // ブレンドなし
            BuildSkinMatrices(resource, currTRS, BoneMatrices);
        }
    }

    // 指定クリップ/時刻の全ボーン LocalTRS を計算する
    void ModelAnimComponent::CalcLocalTRS(
        const graphics::ModelResource& resource,
        int clipIndex, float time,
        std::vector<BoneTRS>& outTRS)
    {
        const auto& bones = resource.GetBones();
        const int   boneCount = static_cast<int>(bones.size());
        outTRS.resize(boneCount);

        // ── レストポーズで初期化 ─────────────────────────────────
        for (int i = 0; i < boneCount; ++i)
        {
            XMMATRIX local = XMLoadFloat4x4(&bones[i].LocalTransform);
            XMVECTOR scale, rot, trans;
            XMMatrixDecompose(&scale, &rot, &trans, local);
            outTRS[i] = { trans, rot, scale };
        }

        const auto& clips = resource.GetAnimClips();
        if (clipIndex < 0 || clipIndex >= static_cast<int>(clips.size())) return;

        const auto& clip = clips[clipIndex];

        if (clip.IsBaked)
        {
            // ── ベイク: インデックス直引き ────────────────────────
            int maxFrame = 0;
            if (!clip.BakedTracks.empty() && !clip.BakedTracks[0].Frames.empty())
                maxFrame = static_cast<int>(clip.BakedTracks[0].Frames.size()) - 1;

            const int frameIdx = std::clamp(
                static_cast<int>(time * clip.BakeFrameRate), 0, maxFrame);

            for (const auto& track : clip.BakedTracks)
            {
                if (track.BoneIndex < 0 || track.BoneIndex >= boneCount) continue;
                if (frameIdx >= static_cast<int>(track.Frames.size())) continue;

                const auto& f = track.Frames[frameIdx];
                outTRS[track.BoneIndex].T = XMLoadFloat3(&f.Translation);
                outTRS[track.BoneIndex].R = XMLoadFloat4(&f.Rotation);
                outTRS[track.BoneIndex].S = XMLoadFloat3(&f.Scale);
            }
        }
        else
        {
            // ── スパース: 補間 ────────────────────────────────────
            for (const auto& track : clip.SparseTracks)
            {
                if (track.BoneIndex < 0 || track.BoneIndex >= boneCount) continue;
                outTRS[track.BoneIndex].T = EvalPosV(track.PosKeys, time);
                outTRS[track.BoneIndex].R = EvalRotV(track.RotKeys, time);
                outTRS[track.BoneIndex].S = EvalScaleV(track.ScaleKeys, time);
            }
        }
    }

    // BuildSkinMatrices 
    void ModelAnimComponent::BuildSkinMatrices(
        const graphics::ModelResource& resource,
        const std::vector<BoneTRS>& trs,
        std::vector<XMFLOAT4X4>& outMatrices)
    {
        const auto& bones = resource.GetBones();
        const int   boneCount = static_cast<int>(bones.size());
        outMatrices.resize(boneCount);

        // テスト：単位行列にしてみる。
        //for (int i = 0; i < boneCount; ++i)
        //{
        //    XMStoreFloat4x4(&outMatrices[i],
        //        XMMatrixTranspose(XMMatrixIdentity()));
        //}

        // グローバル行列 (DFS 順なので親は必ず先に計算済み)
        std::vector<XMMATRIX> globalMats(boneCount, XMMatrixIdentity());

        for (int i = 0; i < boneCount; ++i)
        {
            // TRS → ローカル行列
            XMMATRIX local = XMMatrixAffineTransformation(
                trs[i].S, XMVectorZero(), trs[i].R, trs[i].T);

            // グローバル行列: Child * Parent (行ベクトル方式)
            const int parent = bones[i].ParentIndex;
            globalMats[i] = (parent < 0)
                ? local
                : local * globalMats[parent];

            // スキン行列 = OffsetMatrix * GlobalMatrix (転置して HLSL へ)
            const XMMATRIX offset = XMLoadFloat4x4(&bones[i].OffsetMatrix);
            XMStoreFloat4x4(&outMatrices[i],
                XMMatrixTranspose(offset * globalMats[i]));
        }
    }

    // スパース補間
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
        if (keys.size() == 1) return XMLoadFloat4(&keys.front().value);
        if (t <= keys.front().time) return XMLoadFloat4(&keys.front().value);
        if (t >= keys.back().time)  return XMLoadFloat4(&keys.back().value);

        auto it = std::lower_bound(keys.begin(), keys.end(), t,
            [](const graphics::ModelRotKey& k, float time) { return k.time < time; });
        const auto& hi = *it; const auto& lo = *(it - 1);
        return XMQuaternionSlerp(XMLoadFloat4(&lo.value), XMLoadFloat4(&hi.value),
            (t - lo.time) / (hi.time - lo.time));
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
}


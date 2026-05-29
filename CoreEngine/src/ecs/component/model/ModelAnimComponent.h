#pragma once

#include<vector>
#include<string>
#include<DirectXMath.h>
#include<Utility/Export/Export.h>
#include<graphics/Model/ModelData.h>

namespace graphics
{
	class ModelResource;
}

namespace ecs
{
	struct ENGINE_API ModelAnimComponent
	{
        // 現在クリップ 
        int   CurrentClipIndex = 0;
        float CurrentTime = 0.f;
        float PlaySpeed = 1.f;
        bool  IsLoop = true;
        bool  IsPlaying = true;

        // ブレンド用
        int   PrevClipIndex = -1;    // -1 = ブレンドなし
        float PrevTime = 0.f;
        float BlendTime = 0.f;   // ブレンド経過時間
        float BlendDuration = 0.2f;  // ブレンドに要する秒数

        /// <summary>HLSL 用スキニング行列配列 (転置済み)</summary>
        std::vector<DirectX::XMFLOAT4X4> BoneMatrices;

        // 状態取得
        bool  IsBlending()      const { return PrevClipIndex >= 0; }
        float GetBlendFactor()  const
        {
            return (BlendDuration > 0.f)
                ? std::min(BlendTime / BlendDuration, 1.f) : 1.f;
        }

        /// <summary>インデックス指定で即時再生</summary>
        void Play(int clipIndex = 0, bool loop = true);

        /// <summary>クリップ名指定で即時再生</summary>
        void Play(const graphics::ModelResource& resource,
            const std::string& clipName, bool loop = true);

        /// <summary>
        /// インデックス指定でブレンド切り替え。
        /// 現在のクリップから blendDuration 秒かけてフェードする。
        /// 同クリップを指定した場合は何もしない。
        /// </summary>
        void CrossFade(int newClipIndex,
            float blendDuration = 0.2f, bool loop = true);

        /// <summary>クリップ名指定でブレンド切り替え</summary>
        void CrossFade(const graphics::ModelResource& resource,
            const std::string& clipName,
            float blendDuration = 0.2f, bool loop = true);

        // 再生制御
        void Pause() { IsPlaying = false; }
        void Resume() { IsPlaying = true; }
        void Rewind() { CurrentTime = 0.f; }

        /// <summary>デルタタイムで再生時間を進める (ブレンド時間も更新)</summary>
        void Update(float deltaTime, const graphics::ModelResource& resource);

        /// <summary>
        /// スキニング行列を BoneMatrices に書き込む。
        /// ブレンド中は TRS 空間で補間してから行列を構築する。
        /// </summary>
        void CalcBoneMatrices(const graphics::ModelResource& resource);

    private:
        // 内部 TRS 構造体 
        struct BoneTRS
        {
            DirectX::XMVECTOR T; // 位置
            DirectX::XMVECTOR R; // 回転クォータニオン
            DirectX::XMVECTOR S; // スケール
        };

        /// <summary>指定クリップ/時刻の全ボーン LocalTRS を計算 (レストポーズで初期化済み)</summary>
        static void CalcLocalTRS(
            const graphics::ModelResource& resource,
            int clipIndex, float time,
            std::vector<BoneTRS>& outTRS);

        /// <summary>TRS 配列からスキニング行列を構築して outMatrices に格納</summary>
        static void BuildSkinMatrices(
            const graphics::ModelResource& resource,
            const std::vector<BoneTRS>& trs,
            std::vector<DirectX::XMFLOAT4X4>& outMatrices);

        // スパース補間ヘルパー
        static DirectX::XMVECTOR EvalPosV(const std::vector<graphics::ModelPosKey>& keys, float t);
        static DirectX::XMVECTOR EvalRotV(const std::vector<graphics::ModelRotKey>& keys, float t);
        static DirectX::XMVECTOR EvalScaleV(const std::vector<graphics::ModelScaleKey>& keys, float t);
	};
}
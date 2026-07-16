#pragma once

#include<DirectXMath.h>

namespace ecs
{
    /// <summary>
    /// ダメージ数値のポップアップ表示用ランタイムコンポーネント。
    /// ワールド座標上の位置を保持し、DamageNumberSystemが毎フレーム画面座標へ投影した上で
    /// 上昇・フェードアウトさせる。表示自体は同じエンティティのTextComponentが担当する
    /// （Transformは使わずスクリーン座標を直接持つ既存のTextComponentの流儀に合わせている）。
    /// </summary>
    struct DamageNumberComponent
    {
        /// <summary>現在のワールド座標（毎フレームRiseSpeed分だけYを上昇させる）</summary>
        DirectX::XMFLOAT3 WorldPosition = { 0.0f, 0.0f, 0.0f };

        /// <summary>上昇速度 m/s</summary>
        float RiseSpeed = 20.0f;

        /// <summary>残り表示時間(秒)。0以下になったら破棄する</summary>
        float RemainingTime = 1.0f;

        /// <summary>総表示時間(秒)。フェードアウト計算(RemainingTime / TotalTime)の基準にする</summary>
        float TotalTime = 1.0f;
    };
}

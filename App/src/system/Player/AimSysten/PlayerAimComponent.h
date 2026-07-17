#pragma once

#include<Utility/Export/Export.h>
#include<DirectXMath.h>

namespace ecs
{
    /// <summary>
    /// プレイヤーの攻撃方向（エイム方向）を保持するコンポーネント。
    /// </summary>
    struct ENGINE_API PlayerAimComponent
    {
        /// <summary>
        /// 攻撃方向（正規化済みのワールド空間ベクトル）。
        /// トップダウンのため Y は常に 0。
        /// </summary>
        DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };

        /// <summary>
        /// 攻撃入力（"Attack"アクション＝左クリック）がこのフレームで押されたか。
        /// PlayerInputSystem が毎フレーム更新する。Manual制御の武器のみ参照する
        /// （held ではなく pressed のため、1クリックにつき最大1回だけ発射判定が立つ）。
        /// </summary>
        bool WantsToFire = false;

        /// <summary>
        /// 2つ目の攻撃入力（"Attack2"アクション＝右クリック）がこのフレームで押されたか。
        /// WantsToFire と同様、PlayerInputSystem が毎フレーム更新する。
        /// </summary>
        bool WantsToFireSecondary = false;

        /// <summary>
        /// 3つ目の攻撃入力（"FlickerStrike"アクション＝Rキー/PadY）がこのフレームで押されたか。
        /// WantsToFire と同様、PlayerInputSystem が毎フレーム更新する。
        /// </summary>
        bool WantsToFireTertiary = false;
    };
}
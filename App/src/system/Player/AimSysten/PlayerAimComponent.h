#pragma once

#include<Utility/Export/Export.h>
#include<DirectXMath.h>

namespace ecs
{
    ///<summary>
    ///プレイヤーの攻撃方向、エイム方向を保持するコンポーネント
    ///</summary>
    struct ENGINE_API PlayerAimComponent
    {
        ///<summary>
        ///攻撃方向。正規化済みのワールド空間ベクトルで、トップダウンのためYは常に0
        ///</summary>
        DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };

        ///<summary>
        ///攻撃入力Attackアクション、左クリックがこのフレームで押されたか。PlayerInputSystemが毎フレーム更新し、Manual制御の武器のみ参照する
        ///</summary>
        bool WantsToFire = false;

        ///<summary>
        ///2つ目の攻撃入力Attack2アクション、右クリックがこのフレームで押されたか
        ///</summary>
        bool WantsToFireSecondary = false;

        ///<summary>
        ///3つ目の攻撃入力FlickerStrikeアクション、Rキー/PadYがこのフレームで押されたか
        ///</summary>
        bool WantsToFireTertiary = false;
    };
}
#pragma once

#include <graphics/Effect/Object/EffectObject.h>
#include <Effekseer.h>
#include <DirectXMath.h>
#include <entt/entt.hpp>

namespace ecs
{
    /// <summary>
    /// エフェクト再生コンポーネント
    /// </summary>
    struct EffectComponent
    {
        /// <summary>エフェクトアセット</summary>
        Effekseer::EffectRef Asset;

        /// <summary>再生インスタンス</summary>
        graphics::EffectObject Effect;

        /// <summary>親エンティティ</summary>
        entt::entity Parent = entt::null;

        /// <summary>親からのオフセット座標</summary>
        DirectX::XMFLOAT3 Offset = { 0.f, 0.f, 0.f };

        /// <summary>エフェクト自体のスケール</summary>
        DirectX::XMFLOAT3 Scale = { 1.f, 1.f, 1.f };

        /// <summary>エフェクト自体の回転</summary>
        DirectX::XMFLOAT3 Rotation = { 0.f, 0.f, 0.f };

        /// <summary>ループ再生するか</summary>
        bool IsLoop = false;

        /// <summary>表示するか</summary>
        bool IsVisible = true;
        /// <summary>前回の表示フラグ</summary>
        bool LastIsVisible = false;

        /// <summary>非表示のままにしておく残りシミュレーション時間</summary>
        float HiddenFramesRemaining = 0.f;

        /// <summary>前回適用された位置</summary>
        DirectX::XMFLOAT3 LastAppliedLocation = { 0.f, 0.f, 0.f };
        /// <summary>前回適用された回転</summary>
        DirectX::XMFLOAT3 LastAppliedRotation = { 0.f, 0.f, 0.f };
        /// <summary>前回適用されたスケール</summary>
        DirectX::XMFLOAT3 LastAppliedScale = { 0.f, 0.f, 0.f };
        /// <summary>トランスフォームが適用済みか</summary>
        bool HasAppliedTransform = false;
    };
}
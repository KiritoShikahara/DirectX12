#pragma once

#include<graphics/Effect/Object/EffectObject.h>
#include <Effekseer.h>
#include <DirectXMath.h>
#include <entt/entt.hpp>

namespace ecs
{
    /// <summary>
    /// エフェクト再生コンポーネント。
    /// Transform を持つエンティティに付けると位置・回転・スケールを自動同期する。
    /// Transform なしのエンティティに付けた場合は Offset を位置として使用する。
    /// </summary>
    struct EffectComponent
    {
        /// <summary>エフェクトアセット（EffekseerManager::GetEffect() で取得）</summary>
        Effekseer::EffectRef Asset;

        /// <summary>再生インスタンス</summary>
        graphics::EffectObject Effect;

        /// <summary>
        /// 親エンティティ（Transform を持つ）。
        /// entt::null の場合は自身の Transform を使う。
        /// </summary>
        entt::entity Parent = entt::null;

        /// <summary>親からのオフセット座標</summary>
        DirectX::XMFLOAT3 Offset = { 0.f, 0.f, 0.f };

        /// <summary>
        /// エフェクト自体のスケール。
        /// Transform を持つ場合は Transform.Scale と乗算される。
        /// </summary>
        DirectX::XMFLOAT3 Scale = { 1.f, 1.f, 1.f };

        /// <summary>ループ再生するか</summary>
        bool IsLoop = false;

        /// <summary>表示するか</summary>
        bool IsVisible = true;
        bool LastIsVisible = false;
    };

} // namespace ecs
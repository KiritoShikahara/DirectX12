#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>
#include<vector>

namespace ecs
{
    /// <summary>
    /// DamageNumberComponentを持つエンティティを毎フレーム更新するシステム。
    /// ワールド座標を上昇させ、カメラのViewProjection行列でスクリーン座標へ投影して
    /// TextComponent::X/Yへ反映し、残り時間に応じてフェードアウトさせる。
    /// 表示時間が尽きたエンティティは破棄する。
    /// </summary>
    class DamageNumberSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        // 毎フレームのヒープ確保を避けるため、期限切れエンティティの一時リストを使い回す
        std::vector<entt::entity> mExpired;
    };
}

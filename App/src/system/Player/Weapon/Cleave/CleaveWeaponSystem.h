#pragma once

#include<entt/entt.hpp>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct CleaveWeaponData; }

namespace ecs
{
    struct WeaponComponent;

    ///<summary>
    ///Cleave型武器の発動ロジック。FireInterval秒ごとに狙い方向を中心とした扇状範囲内の敵全員へ近接ダメージとノックバックを与える完全自動の武器
    ///</summary>
    class CleaveWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///狙い方向の扇状範囲内にいる敵全員へダメージ・ノックバックを与える
        ///</summary>
        void Swing(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const data::CleaveWeaponData& masterData);

        ///<summary>
        ///SwingのOverlapSphere結果の一時バッファ、毎回clearして再利用する
        ///</summary>
        std::vector<entt::entity> mOverlapped;
    };
}

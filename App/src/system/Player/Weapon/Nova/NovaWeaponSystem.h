#pragma once

#include<entt/entt.hpp>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs { struct WeaponComponent; }
namespace data { struct NovaWeaponData; }

namespace ecs
{
    ///<summary>
    ///Nova型武器を処理するシステム。プレイヤー自身を中心とした周期的な範囲ダメージを与える完全自動の持続武器
    ///</summary>
    class NovaWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///所有者中心に球形ダメージを与え、ワンショットエフェクトを再生する
        ///</summary>
        void Pulse(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const data::NovaWeaponData& masterData);

        ///<summary>
        ///PulseのOverlapSphere結果の一時バッファ、毎回clearして再利用する
        ///</summary>
        std::vector<entt::entity> mOverlapped;
    };
}

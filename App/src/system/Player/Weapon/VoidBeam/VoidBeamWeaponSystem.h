#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct VoidBeamWeaponData; }

namespace ecs
{
    struct WeaponComponent;

    ///<summary>
    ///VoidBeam型武器の発動ロジック。最も近い敵の方向へ直線状の貫通ビームを放つ完全自動の武器
    ///</summary>
    class VoidBeamWeaponSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        ///<summary>
        ///directionへ向けてBeamLength・BeamWidthの直線範囲内にいる敵全員へダメージを与える
        ///</summary>
        void Fire(
            entt::registry& registry,
            const ecs::WeaponComponent& weapon,
            const DirectX::XMFLOAT3& origin,
            const DirectX::XMFLOAT3& direction,
            const data::VoidBeamWeaponData& masterData);

        ///<summary>
        ///FireのOverlapSphere結果の一時バッファ、毎回clearして再利用する
        ///</summary>
        std::vector<entt::entity> mCandidates;
    };
}

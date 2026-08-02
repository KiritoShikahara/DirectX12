#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct AreaAttackWeaponData; }

namespace ecs
{
	struct WeaponComponent;

	///<summary>
	///AreaAttack型武器の自動発動ロジック。手動発動とは独立したクールダウンでプレイヤー周囲の敵へ自動で氷柱を落とす
	///</summary>
	class AreaAttackAutoStrikeSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///SearchRadius内の敵からランダムに最大AutoStrikeCount体を選び氷柱を落とす、不発ならfalseを返す
		///</summary>
		bool Fire(
			entt::registry& registry,
			const ecs::WeaponComponent& weapon,
			const data::AreaAttackWeaponData& masterData);

		///<summary>
		///FireのOverlapSphere結果/敵フィルタ結果の一時バッファ、毎回clearして再利用する
		///</summary>
		std::vector<entt::entity> mFound;
		std::vector<entt::entity> mEnemies;
	};
}

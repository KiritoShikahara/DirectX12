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
	///AreaAttack型武器の発動ロジック。狙い方向の地点を中心に敵を検出し、各敵の座標へ氷柱を生成する。右クリックで発動するManual制御武器
	///</summary>
	class AreaAttackWeaponSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///探索範囲センサーを毎フレーム可視化する、Physics Debug表示用
		///</summary>
		static void UpdateSearchAreaVisual(
			entt::registry& registry,
			entt::entity weaponEntity,
			const ecs::WeaponComponent& weapon,
			const data::AreaAttackWeaponData& masterData);

		///<summary>
		///狙い方向の範囲内から敵を検出し、各敵の座標へ氷柱ハザードを生成する
		///</summary>
		void Fire(
			entt::registry& registry,
			const ecs::WeaponComponent& weapon,
			const data::AreaAttackWeaponData& masterData);

		///<summary>
		///FireのOverlapSphere結果の一時バッファ、毎回clearして再利用する
		///</summary>
		std::vector<entt::entity> mFound;
	};
}

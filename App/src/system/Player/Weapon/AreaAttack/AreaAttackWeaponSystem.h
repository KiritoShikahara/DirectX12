#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct AreaAttackWeaponData; }

namespace ecs
{
	struct WeaponComponent;

	/// <summary>
	/// AreaAttack型武器の発動ロジック。狙い方向へForwardOffset離れた地点を中心に
	/// SearchRadius内の敵を最大MaxTargets体検出し、各敵の座標へ氷柱(ハザード)を生成する。
	/// 氷柱の持続時間管理・ダメージ反復はAreaAttackHazardSystemが担当する。
	/// 右クリック("Attack2")で発動するManual制御(SingleShotと対の初期武器)。
	/// </summary>
	class AreaAttackWeaponSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>探索範囲(センサー)を毎フレーム可視化する(Physics Debug表示用)</summary>
		static void UpdateSearchAreaVisual(
			entt::registry& registry,
			entt::entity weaponEntity,
			const ecs::WeaponComponent& weapon,
			const data::AreaAttackWeaponData& masterData);

		/// <summary>狙い方向の範囲内から敵を検出し、各敵の座標へ氷柱(ハザード)を生成する</summary>
		void Fire(
			entt::registry& registry,
			const ecs::WeaponComponent& weapon,
			const data::AreaAttackWeaponData& masterData);

		// Fire()のOverlapSphere結果の一時バッファ。毎回clear()して再利用する
		std::vector<entt::entity> mFound;
	};
}

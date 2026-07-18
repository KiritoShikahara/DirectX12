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
	/// AreaAttack型武器(WeaponComponent::Type == AreaAttack)の発動ロジック。
	/// 所有者(Owner)の PlayerAimComponent::Direction（マウス座標/右スティックでの狙い方向。
	/// SingleShotと同じ基準）へ ForwardOffset だけ離れた地点を中心に、SearchRadius内から
	/// 敵を最大MaxTargets体まで検出し、各敵の座標へ個別に氷柱(ハザード)を生成する。
	/// 氷柱自体の持続時間管理・ダメージ反復適用は AreaAttackHazardSystem が担当する
	/// （発動時の対象選定と、個々のハザードのライフサイクルを分離する設計）。
	///
	/// SingleShotと同じ初期武器のため、右クリック("Attack2"アクション)で発動するManual制御。
	/// </summary>
	class AreaAttackWeaponSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>
		/// 探索範囲(センサー)を毎フレーム可視化する。発動の成否・クールダウンに関わらず、
		/// 「どこまでが検出範囲か」を常に表示する(ImGui「Physics Debug」→「Show Colliders」)。
		/// 武器エンティティ自体にTransform/DebugWireSphereComponentを持たせ、狙い方向に
		/// 追従させる（武器エンティティは本来Transformを持たないため、初回のみemplaceする）。
		/// </summary>
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

		/// <summary>1体の敵の座標に氷柱(ハザード)エンティティを1体生成する</summary>
		static void SpawnHazard(
			entt::registry& registry,
			const DirectX::XMFLOAT3& position,
			float radius,
			float damage,
			const data::AreaAttackWeaponData& masterData);
	};
}

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
	/// AreaAttack型武器(IceSpike)の自動発動ロジック。手動発動(右クリック"Attack2"、
	/// AreaAttackWeaponSystemが担当)とは独立したクールダウンで、プレイヤー周囲の
	/// SearchRadius内からランダムに最大AutoStrikeCount体の敵を選び、各敵の座標へ
	/// 氷柱(ハザード)を自動で落とす。ダメージ/半径等は手動発動と全く同じマスタデータを使う。
	/// 狙い不要で自動的に発動する(Meteor等の自動発動武器と同じ方針)。
	/// </summary>
	class AreaAttackAutoStrikeSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>発動: SearchRadius内の敵からランダムに最大AutoStrikeCount体を選び、
		/// それぞれの座標へ氷柱(ハザード)を落とす。
		/// 戻り値: 対象が1体も見つからず不発だった場合はfalse
		/// （クールダウンを消費せず待機させるため、呼び出し側が判定に使う）</summary>
		bool Fire(
			entt::registry& registry,
			const ecs::WeaponComponent& weapon,
			const data::AreaAttackWeaponData& masterData);

		// Fire()のOverlapSphere結果/敵フィルタ結果の一時バッファ。毎回clear()して再利用する
		std::vector<entt::entity> mFound;
		std::vector<entt::entity> mEnemies;
	};
}

#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>
#include<DirectXMath.h>
#include<vector>

namespace ecs { struct WeaponComponent; }
namespace data { struct MeteorWeaponData; }

namespace ecs
{
	/// <summary>
	/// Meteor型武器（周囲の敵複数体の頭上へ隕石を落とす範囲攻撃）を処理するシステム。
	/// 狙い不要で自動的に発動する完全自動の武器。InGame状態のときのみ動作する。
	/// </summary>
	class MeteorWeaponSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>発動: SearchRadius内の敵からランダムに最大MeteorCount体を選び、
		/// それぞれの座標へ隕石(範囲ダメージ+エフェクト)を落とす。
		/// 戻り値: 対象が1体も見つからず不発だった場合はfalse
		/// （クールダウンを消費せず待機させるため、呼び出し側が判定に使う）</summary>
		bool Fire(
			entt::registry& registry,
			const ecs::WeaponComponent& weapon,
			const data::MeteorWeaponData& masterData);

		/// <summary>1体分の隕石落下：範囲ダメージを与えワンショットエフェクトを再生する</summary>
		void Strike(
			entt::registry& registry,
			const DirectX::XMFLOAT3& position,
			float hitRadius,
			float damage,
			float visualRadius,
			const data::MeteorWeaponData& masterData);

		// Fire()のOverlapSphere結果/敵フィルタ結果の一時バッファ。毎回clear()して再利用する
		std::vector<entt::entity> mFound;
		std::vector<entt::entity> mEnemies;
		// Strike()のOverlapSphere結果の一時バッファ(隕石1発ごとにclear()して再利用)
		std::vector<entt::entity> mOverlapped;
	};
}

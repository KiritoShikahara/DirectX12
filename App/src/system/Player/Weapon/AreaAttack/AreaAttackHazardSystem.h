#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// AreaAttackHazardComponent(設置済みの氷柱)の持続時間管理とダメージ反復適用を行う。
	/// - 毎フレーム RemainingDuration を減らし、0以下になったエンティティを破棄する。
	/// - TickTimer が0以下になるたびに Radius 範囲内の敵へ Damage を与え、TickIntervalへリセットする。
	/// </summary>
	class AreaAttackHazardSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>指定範囲内の敵にダメージを与える（敵タグ以外は無視する）</summary>
		void ApplyTickDamage(
			entt::registry& registry,
			const DirectX::XMFLOAT3& center,
			float radius,
			float damage);

		// Update()内: 破棄対象の一時バッファ。毎回clear()して再利用する
		std::vector<entt::entity> mExpired;
		// ApplyTickDamage()のOverlapSphere結果の一時バッファ(ハザード1件ごとにclear()して再利用)
		std::vector<entt::entity> mOverlapped;
	};
}

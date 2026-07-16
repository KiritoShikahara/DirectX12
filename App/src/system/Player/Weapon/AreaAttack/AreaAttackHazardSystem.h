#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
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
		static void ApplyTickDamage(
			entt::registry& registry,
			const DirectX::XMFLOAT3& center,
			float radius,
			float damage);
	};
}

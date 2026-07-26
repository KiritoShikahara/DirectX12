#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
	/// <summary>
	/// EnemySlowStatusComponentの残り時間を毎フレーム減らし、尽きたコンポーネントを外して
	/// 通常速度へ戻すシステム。減速の付与自体は各武器システム(現状OrbitWeaponSystemのみ)が
	/// 行うため、このシステムは時間経過による解除だけに責務を絞る。
	/// </summary>
	class EnemySlowStatusSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		// 期限切れエンティティの一時バッファ。view走査中の直接removeはイテレータを壊しうるため
		// 走査完了後にまとめて外す。毎フレームのvector生成を避けるためメンバで使い回す
		std::vector<entt::entity> mExpired;
	};
}

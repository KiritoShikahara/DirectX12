#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<vector>
#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///AreaAttackHazardComponentの持続時間管理とダメージ反復適用を行う
	///</summary>
	class AreaAttackHazardSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///指定範囲内の敵にダメージを与える、敵タグ以外は無視する
		///</summary>
		void ApplyTickDamage(
			entt::registry& registry,
			const DirectX::XMFLOAT3& center,
			float radius,
			float damage);

		///<summary>
		///Updateの一時バッファ、破棄対象。毎回clearして再利用する
		///</summary>
		std::vector<entt::entity> mExpired;

		///<summary>
		///ApplyTickDamageのOverlapSphere結果の一時バッファ、ハザード1件ごとにclearして再利用する
		///</summary>
		std::vector<entt::entity> mOverlapped;
	};
}

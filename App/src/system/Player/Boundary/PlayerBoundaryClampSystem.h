#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///プレイヤーがフィールド境界の外に出ないよう毎フレーム位置をクランプする最終防衛ライン。Flicker Strikeのワープ等、壁際の敵座標を無条件に信用した瞬間移動が境界外へ出てしまう経路を塞ぐ
	///</summary>
	class PlayerBoundaryClampSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}

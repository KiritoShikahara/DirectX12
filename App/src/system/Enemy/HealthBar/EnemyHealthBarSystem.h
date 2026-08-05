#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
	///<summary>
	///敵の頭上体力バー(EnemyHealthBarTagが付いたSprite)を毎フレーム更新する。
	///Ownerのワールド座標をスクリーン座標へ投影して追従させ、残量をFillAmountへ反映する。
	///Ownerが破棄されたバーはここで一緒に破棄する
	///</summary>
	class EnemyHealthBarSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>Ownerを失った(敵が破棄された)ため今フレームで破棄するバー一覧。毎フレーム使い回す</summary>
		std::vector<entt::entity> mExpired;
	};
}

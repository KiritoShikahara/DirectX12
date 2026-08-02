#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///ハブ画面の選択肢移動・確定・キャンセルを処理する。MenuLeft/MenuRightで移動、Selectで確定してMenuSceneかStatusUpgradeSceneへ、CancelでTitleSceneへ戻る
	///</summary>
	class HubMenuInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}

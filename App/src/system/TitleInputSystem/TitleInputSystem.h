#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>タイトル画面の操作案内テキストに付与するタグ。「Selectって何ボタン？」と
	/// ならないよう、TitleInputSystemが最後に使われた入力デバイスに応じて毎フレーム内容を更新する</summary>
	struct TitleGuideUiTag
	{
	};
}

namespace sys
{
	/// <summary>
	/// 入力されたらタイトル画面から次の画面に遷移する。
	/// </summary>
	class TitleInputSystem : public ::ecs::IUserSystem
	{
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}



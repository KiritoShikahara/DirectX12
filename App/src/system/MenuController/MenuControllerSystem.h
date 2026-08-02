#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///MenuSlideComp::TargetXへTransformの2D位置を補間させるシステム
	///</summary>
	class MenuSlideSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};

	///<summary>
	///MenuControllerComp::CurrentlySelectedIdxを基準に各ページのMenuSlideComp::TargetXとMenuControllerComp::ActiveSpellIDを毎フレーム再計算するシステム。MenuInputSystemの後に実行すること
	///</summary>
	class MenuPagingSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};

	///<summary>
	///左右入力を読んでMenuControllerComp::CurrentlySelectedIdxを更新するシステム
	///</summary>
	class MenuInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};

	///<summary>
	///メニュー画面の入力からの画面遷移管理
	///</summary>
	class MenuSelectInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}

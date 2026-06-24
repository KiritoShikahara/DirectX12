#pragma once

#include<ecs/system/manager/IComponentSystem.h>

// システムの順番は
// PreUpdate : MenuInputSystem
// Update : MenuPagingSystem  → MenuSlideSystem

namespace ecs
{
	/// <summary>
	/// MenuSlideComp::TargetX へ Transform の2D位置を補間させるシステム。
	/// </summary>
	class MenuSlideSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};

	/// <summary>
	/// MenuControllerComp::CurrentlySelectedIdx を基準に、
	/// 各ページの MenuSlideComp::TargetX と
	/// MenuControllerComp::ActiveSpellID を毎フレーム再計算するシステム。
	/// </summary>
	class MenuPagingSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};

	/// <summary>
	/// 左右入力を読んで MenuControllerComp::CurrentlySelectedIdx を更新するシステム。
	/// </summary>
	class MenuInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};

}
	 



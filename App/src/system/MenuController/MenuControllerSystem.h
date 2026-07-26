#pragma once

#include<ecs/system/manager/IComponentSystem.h>

// 繧ｷ繧ｹ繝・Β縺ｮ鬆・分縺ｯ
// PreUpdate : MenuInputSystem
// Update : MenuPagingSystem  竊・MenuSlideSystem

namespace ecs
{
	/// <summary>
	/// MenuSlideComp::TargetX 縺ｸ Transform 縺ｮ2D菴咲ｽｮ繧定｣憺俣縺輔○繧九す繧ｹ繝・Β縲・
	/// </summary>
	class MenuSlideSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};

	/// <summary>
	/// MenuControllerComp::CurrentlySelectedIdx 繧貞渕貅悶↓縲・
	/// 蜷・・繝ｼ繧ｸ縺ｮ MenuSlideComp::TargetX 縺ｨ
	/// MenuControllerComp::ActiveSpellID 繧呈ｯ弱ヵ繝ｬ繝ｼ繝蜀崎ｨ育ｮ励☆繧九す繧ｹ繝・Β縲・
	/// </summary>
	class MenuPagingSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};

	/// <summary>
	/// 蟾ｦ蜿ｳ蜈･蜉帙ｒ隱ｭ繧薙〒 MenuControllerComp::CurrentlySelectedIdx 繧呈峩譁ｰ縺吶ｋ繧ｷ繧ｹ繝・Β縲・
	/// </summary>
	class MenuInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};

	/// <summary>
	/// 繝｡繝九Η繝ｼ逕ｻ髱｢縺ｮ蜈･蜉帙°繧峨・逕ｻ髱｢驕ｷ遘ｻ邂｡逅・
	/// </summary>
	class MenuSelectInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
	 



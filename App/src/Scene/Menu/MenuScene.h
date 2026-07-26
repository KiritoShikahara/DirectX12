#pragma once

#include<system/Scene/IScene.h>
#include<entt/entt.hpp>

namespace scene
{
	class MenuScene : public ::sys::IScene
	{
	public:
		virtual void Initialize()override;
		virtual void Finalize()override;
	private:
		// 繝・・繧ｿ
		void LoadData();

		// 繧ｷ繧ｹ繝・Β縺ｮ霑ｽ蜉
		void CreateUserSystem();

		// 閭梧勹
		void CreateBG();

		// 繧ｹ繝壹Ν縺ｮ菴懈・
		void CreateSpells();

	};
}
	 



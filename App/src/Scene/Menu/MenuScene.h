#pragma once

#include<system/Scene/IScene.h>

namespace scene
{
	class MenuScene : public ::sys::IScene
	{
	public:
		virtual void Initialize()override;
		virtual void Finalize()override;
	private:
		// データ
		void LoadData();

		// システムの追加
		void CreateUserSystem();

		// 背景
		void CreateBG();

		// スペルの作成
		void CreateSpells();

	};
}
	 



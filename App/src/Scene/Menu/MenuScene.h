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
		// リソース

		// データ

		// 背景
		void CreateBG();


	};
}
	 



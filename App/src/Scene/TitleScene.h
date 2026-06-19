#pragma once

#include<system/Scene/IScene.h>

namespace scene
{
	/// <summary>
	/// タイトル画面のシーン
	/// </summary>
	class TitleScene : public ::sys::IScene
	{
	public:
		virtual void Initialize()override;
		virtual void Finalize()override;

	private:

	};
}
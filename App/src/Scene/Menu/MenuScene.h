#pragma once

#include <system/Scene/IScene.h>
#include <entt/entt.hpp>

namespace scene
{
	/// <summary>
	/// メニュー画面
	/// </summary>
	class MenuScene : public ::sys::IScene
	{
	public:
		virtual void Initialize() override;
		virtual void Finalize() override;

	private:
		/// <summary>
		/// データ読み込み
		/// </summary>
		void LoadData();

		/// <summary>
		/// ユーザーシステムの追加
		/// </summary>
		void CreateUserSystem();

		/// <summary>
		/// 背景の生成
		/// </summary>
		void CreateBG();

		/// <summary>
		/// スペルの生成
		/// </summary>
		void CreateSpells();
	};
}
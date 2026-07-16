#pragma once

#include<system/Scene/IScene.h>

namespace scene
{
	/// <summary>
	/// タイトルとゲーム本編の間に挟まるハブ画面。
	/// 「武器・ステージ選択(MenuScene)」「ステータス強化(StatusUpgradeScene)」の
	/// 2択から選んで遷移する。
	/// </summary>
	class HubScene : public ::sys::IScene
	{
	public:
		virtual void Initialize()override;
		virtual void Finalize()override;

	private:
		/// <summary>
		/// リソース読み込み
		/// </summary>
		static void LoadResource();

		/// <summary>
		/// ハブ画面で必要なシステム
		/// </summary>
		static void CreateCompSystem();

		/// <summary>
		/// 背景
		/// </summary>
		static void CreateBackground();

		/// <summary>
		/// 選択肢(武器・ステージ選択/ステータス強化)
		/// </summary>
		static void CreateOptions();
	};
}

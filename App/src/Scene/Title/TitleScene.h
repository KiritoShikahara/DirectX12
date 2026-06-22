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
		/// <summary>
		/// リソース読み込み
		/// </summary>
		static void LoadResource();

		/// <summary>
		/// タイトルで必要なシステム
		/// </summary>
		static void CreateCompSystem();

		/// <summary>
		/// 背景
		/// </summary>
		static void CreateBackground();

		/// <summary>
		/// ロゴ
		/// </summary>
		static void CreateLogo();

		/// <summary>
		/// 操作誘導テキスト
		/// </summary>
		static void CreatePromptText();

	};
}
#pragma once

#include<system/Scene/IScene.h>
#include<Data/Save/PlayerSaveDebugPanel.h>

#include<memory>

namespace scene
{
	/// <summary>
	/// ステータス強化画面。
	/// PlayerSaveData(ゴールド所持数・各ステータスの強化レベル、JSON永続化)を消費して
	/// MaxHp/AtkPower/Defense/CooldownRateを恒久的に強化する。強化コスト・効果量・
	/// レベル上限はStatUpgradeData(CSV/DB)で管理する。
	/// MenuUp/MenuDownで選択、Selectで購入、CancelでHubSceneへ戻る
	/// （操作・購入処理はStatusUpgradeInputSystemが担当する）。
	/// </summary>
	class StatusUpgradeScene : public ::sys::IScene
	{
	public:
		virtual void Initialize()override;
		virtual void Finalize()override;

	private:
		/// <summary>
		/// データ読み込み(StatUpgradeData/PlayerSaveData)
		/// </summary>
		static void LoadData();

		/// <summary>
		/// リソース読み込み
		/// </summary>
		static void LoadResource();

		/// <summary>
		/// ステータス強化画面で必要なシステム
		/// </summary>
		static void CreateCompSystem();

		/// <summary>
		/// 背景
		/// </summary>
		static void CreateBackground();

		/// <summary>
		/// 見出し・所持ゴールド・4ステータス分の選択肢テキストを生成する
		/// </summary>
		static void CreateOptions();

		std::unique_ptr<debug::PlayerSaveDebugPanel> mPlayerSaveDebugPanel;
	};
}

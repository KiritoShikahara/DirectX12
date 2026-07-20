#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<string>

namespace data { struct StatUpgradeData; }

namespace ecs
{
	struct StatusUpgradeComponent;

	/// <summary>
	/// ステータス強化画面(StatusUpgradeScene)の一連の処理を担当する。
	/// - MenuUp/MenuDownでカーソル移動
	/// - Selectで「強化しますか？」の確認ダイアログを開く（ゴールド不足・レベル上限の場合は
	///   ダイアログを開かずフィードバックメッセージを表示する）
	/// - 確認ダイアログ中：Selectで確定(強化レベル+1・PlayerSaveDataへ即座に保存)、
	///   Cancelで確認を取り消す(コストはStatUpgradeData::BaseCost + CostGrowthPerLevel×
	///   現在レベルで、レベルが上がるごとに増加する)
	/// - 確認ダイアログ非表示中のCancelで HubScene へ戻る
	/// - 所持ゴールド・各行の表示(レベル/コスト)・確認ダイアログ・フィードバックメッセージは
	///   毎フレーム最新の状態で更新する
	/// </summary>
	class StatusUpgradeInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		static void RefreshTexts(entt::registry& registry, const StatusUpgradeComponent& upgrade);
		static void TryOpenConfirm(StatusUpgradeComponent& upgrade);
		static void ConfirmPurchase(StatusUpgradeComponent& upgrade);
		static void ShowMessage(StatusUpgradeComponent& upgrade, std::wstring message);

		/// <summary>
		/// 選択中の項目を、所持ゴールドで買える範囲かつ最大レベルまで一気に強化する。
		/// 1レベルずつ何度も確認ダイアログを挟むのが煩雑なため用意する。
		/// </summary>
		static void PurchaseMaxLevel(StatusUpgradeComponent& upgrade);

		/// <summary>
		/// 全ステータスの強化レベルを0へ戻し、消費したゴールドを全額払い戻す。
		/// 振り直しができないと構成を試せないため用意する
		/// (払い戻し額は購入時と同じ計算式で求めるため、損得は発生しない)。
		/// </summary>
		static void ResetAllUpgrades(StatusUpgradeComponent& upgrade);

		/// <summary>指定項目を現在レベルから1つ上げるのに必要なゴールド</summary>
		static int ComputeCost(const data::StatUpgradeData& upgradeData, int currentLevel);
	};
}

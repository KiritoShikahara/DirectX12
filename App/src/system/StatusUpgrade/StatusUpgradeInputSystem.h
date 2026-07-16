#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<string>

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
	};
}

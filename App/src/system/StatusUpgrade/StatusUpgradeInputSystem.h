#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<string>

namespace data { struct StatUpgradeData; }

namespace ecs
{
	struct StatusUpgradeComponent;

	/// <summary>
	/// ステータス強化画面(StatusUpgradeScene)の一連の処理を担当する。
	/// - WASD/十字キー(MenuUp/Down/Left/Right)で横4×縦2グリッドのカーソル移動
	/// - Space(Select)で選択中の項目を1レベルだけ即座に強化する(確認ダイアログ無し。
	///   ゴールド不足・レベル上限の場合はフィードバックメッセージのみ表示する)
	/// - Enter(SelectAll)で「最大レベルまで強化」の確認ダイアログを開く。
	///   現在レベル→到達可能レベル(所持ゴールドで買える範囲)、所持ゴールド→強化後のゴールドを
	///   ダイアログに表示する。1レベルも買えない場合はダイアログ内に「ゴールドが足りません」と表示する
	/// - 確認ダイアログ中：Selectで確定(到達可能レベルまで一括購入)、Cancelで取り消す
	/// - 確認ダイアログ非表示中のCancelで HubScene へ戻る
	/// - 所持ゴールド・各カードの表示(レベル/コスト)・確認ダイアログ・フィードバックメッセージは
	///   毎フレーム最新の状態で更新する
	/// </summary>
	class StatusUpgradeInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		static void RefreshTexts(entt::registry& registry, const StatusUpgradeComponent& upgrade);

		/// <summary>グリッド上でのカーソル移動(列・行それぞれ独立にラップする)</summary>
		static void MoveCursor(StatusUpgradeComponent& upgrade, int colDelta, int rowDelta);

		/// <summary>選択中の項目を1レベルだけ即座に強化する(確認ダイアログ無し)。
		/// レベル上限・ゴールド不足の場合はフィードバックメッセージのみ表示する</summary>
		static void PurchaseOneLevel(StatusUpgradeComponent& upgrade);

		/// <summary>「最大レベルまで強化しますか？」の確認ダイアログを開く。
		/// レベル上限の場合はダイアログを開かずフィードバックメッセージを表示する
		/// (ゴールド不足の場合は、あえてダイアログを開いた上で内容を「ゴールドが足りません」にする。
		/// ユーザー要望により、ダイアログ表示自体は常時行う仕様のため)</summary>
		static void TryOpenMaxConfirm(StatusUpgradeComponent& upgrade);

		/// <summary>確認ダイアログで「はい」が選ばれた時の実際の購入処理。
		/// 所持ゴールドで買える範囲かつ最大レベルまで一気に強化する</summary>
		static void ConfirmMaxPurchase(StatusUpgradeComponent& upgrade);

		static void ShowMessage(StatusUpgradeComponent& upgrade, std::wstring message);

		/// <summary>
		/// 全ステータスの強化レベルを0へ戻し、消費したゴールドを全額払い戻す。
		/// 振り直しができないと構成を試せないため用意する
		/// (払い戻し額は購入時と同じ計算式で求めるため、損得は発生しない)。
		/// </summary>
		static void ResetAllUpgrades(StatusUpgradeComponent& upgrade);

		/// <summary>指定項目を現在レベルから1つ上げるのに必要なゴールド</summary>
		static int ComputeCost(const data::StatUpgradeData& upgradeData, int currentLevel);

		/// <summary>現在レベルから、所持ゴールドで買える範囲の到達可能レベルと、
		/// それに必要な合計コストを求める(実際には購入しない、確認ダイアログ用のシミュレーション)</summary>
		static void SimulateMaxPurchase(const data::StatUpgradeData& upgradeData, int currentLevel, int gold,
			int& outTargetLevel, int& outTotalCost);
	};
}

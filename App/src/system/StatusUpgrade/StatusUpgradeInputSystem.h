#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<string>

namespace data { struct StatUpgradeData; }

namespace ecs
{
	struct StatusUpgradeComponent;

	///<summary>
	///ステータス強化画面の一連の処理を担当する。カーソル移動、Selectでの即時1レベル強化、SelectAllでの最大まで強化の確認ダイアログ、Cancelでの取り消しやHubSceneへの復帰を行い、表示は毎フレーム最新の状態に更新する
	///</summary>
	class StatusUpgradeInputSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		static void RefreshTexts(entt::registry& registry, const StatusUpgradeComponent& upgrade);

		///<summary>
		///グリッド上でのカーソル移動。列・行それぞれ独立にラップする
		///</summary>
		static void MoveCursor(StatusUpgradeComponent& upgrade, int colDelta, int rowDelta);

		///<summary>
		///選択中の項目を1レベルだけ即座に強化する。確認ダイアログ無し。レベル上限・ゴールド不足の場合はフィードバックメッセージのみ表示する
		///</summary>
		static void PurchaseOneLevel(StatusUpgradeComponent& upgrade);

		///<summary>
		///最大レベルまで強化しますかの確認ダイアログを開く。レベル上限の場合はダイアログを開かずフィードバックメッセージを表示する。ゴールド不足の場合はあえてダイアログを開いた上で内容をゴールドが足りませんにする
		///</summary>
		static void TryOpenMaxConfirm(StatusUpgradeComponent& upgrade);

		///<summary>
		///確認ダイアログではいが選ばれた時の実際の購入処理。所持ゴールドで買える範囲かつ最大レベルまで一気に強化する
		///</summary>
		static void ConfirmMaxPurchase(StatusUpgradeComponent& upgrade);

		static void ShowMessage(StatusUpgradeComponent& upgrade, std::wstring message);

		///<summary>
		///全ステータスの強化レベルを0へ戻し、消費したゴールドを全額払い戻す。払い戻し額は購入時と同じ計算式で求めるため損得は発生しない
		///</summary>
		static void ResetAllUpgrades(StatusUpgradeComponent& upgrade);

		///<summary>
		///指定項目を現在レベルから1つ上げるのに必要なゴールド
		///</summary>
		static int ComputeCost(const data::StatUpgradeData& upgradeData, int currentLevel);

		///<summary>
		///現在レベルから、所持ゴールドで買える範囲の到達可能レベルと、それに必要な合計コストを求める。実際には購入しない、確認ダイアログ用のシミュレーション
		///</summary>
		static void SimulateMaxPurchase(const data::StatUpgradeData& upgradeData, int currentLevel, int gold,
			int& outTargetLevel, int& outTotalCost);
	};
}

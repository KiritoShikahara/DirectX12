#pragma once

#include<string>

namespace ecs
{
	/// <summary>
	/// ステータス強化画面(StatusUpgradeScene)のカーソル位置・確認ダイアログ・
	/// フィードバックメッセージの状態を保持するコンポーネント。
	/// 選択肢はdata::eStatUpgradeTypeと対応する固定4件(MaxHp/AtkPower/Defense/CooldownRate)。
	/// </summary>
	struct StatusUpgradeComponent
	{
		static constexpr int kOptionCount = 8;

		/// <summary>現在カーソルが当たっている選択肢(0..kOptionCount-1、data::eStatUpgradeTypeと対応)</summary>
		int SelectedIndex = 0;

		/// <summary>「強化しますか？」の確認ダイアログを表示中か。true中はMenuUp/MenuDownでの
		/// カーソル移動とCancelでのHubSceneへの遷移を止め、Select=はい/Cancel=いいえの
		/// 確認操作だけを受け付ける</summary>
		bool IsConfirming = false;

		/// <summary>フィードバックメッセージ(「ゴールドが足りません」等)の残り表示時間(秒)。
		/// 0以下なら非表示</summary>
		float MessageTimer = 0.0f;

		/// <summary>表示中のフィードバックメッセージ本文</summary>
		std::wstring Message;
	};

	/// <summary>ステータス強化画面の各行(1ステータス分)のテキストに付与するタグ</summary>
	struct StatusUpgradeOptionUiTag
	{
		/// <summary>対応する選択肢インデックス(0..StatusUpgradeComponent::kOptionCount-1)</summary>
		int OptionIndex = 0;
	};

	/// <summary>所持ゴールド表示テキストに付与するタグ</summary>
	struct StatusUpgradeGoldUiTag
	{
	};

	/// <summary>「強化しますか？」確認ダイアログ表示テキストに付与するタグ</summary>
	struct StatusUpgradeConfirmUiTag
	{
	};

	/// <summary>フィードバックメッセージ表示テキストに付与するタグ</summary>
	struct StatusUpgradeMessageUiTag
	{
	};
}

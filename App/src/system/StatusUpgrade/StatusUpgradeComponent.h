#pragma once

#include<string>

namespace ecs
{
	/// <summary>
	/// ステータス強化画面(StatusUpgradeScene)のカーソル位置・確認ダイアログ・
	/// フィードバックメッセージの状態を保持するコンポーネント。
	/// 選択肢はdata::eStatUpgradeTypeと対応する固定12件(kOptionCount参照)。
	/// </summary>
	struct StatusUpgradeComponent
	{
		static constexpr int kOptionCount = 12;

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

	/// <summary>ステータス強化画面の1カード(アイコン+名前+強化状態)を構成する要素の種類。
	/// レイアウトは「名前(上)→アイコン(中)→レベル等の強化状態(下)」の縦積み</summary>
	enum class eStatusUpgradeCardElement
	{
		NameText,  // ステータス名(アイコンの上)
		Icon,      // アイコン画像本体
		StateText, // Lv./コスト等の強化状態(アイコンの下)
	};

	/// <summary>
	/// ステータス強化画面の1カード(1ステータス分)を識別するタグ。
	/// NameText/Icon/StateTextの3エンティティが同じOptionIndexを共有する(Elementで区別する)。
	/// </summary>
	struct StatusUpgradeCardUiTag
	{
		/// <summary>対応する選択肢インデックス(0..StatusUpgradeComponent::kOptionCount-1、data::eStatUpgradeTypeと対応)</summary>
		int OptionIndex = 0;
		eStatusUpgradeCardElement Element = eStatusUpgradeCardElement::Icon;

		/// <summary>
		/// Element==NameText/StateTextの場合のみ使用。テキストの水平中央揃えの基準X座標(不変)。
		/// TextComponent::Xはテキスト幅に応じて毎フレーム書き換えられる(中央揃えのため)ため、
		/// 基準座標をここへ別途保持しておかないと、書き換え後の値を元に再計算してしまい
		/// 位置がズレ続けるバグになる(WeaponIconSlotTag::CenterXと同じ理由)。
		/// </summary>
		float CenterX = 0.0f;
	};

	/// <summary>所持ゴールド表示テキストに付与するタグ</summary>
	struct StatusUpgradeGoldUiTag
	{
	};

	/// <summary>「強化しますか？」確認ダイアログ表示テキストに付与するタグ</summary>
	struct StatusUpgradeConfirmUiTag
	{
	};

	/// <summary>確認ダイアログの背景(黒半透明、画面中心)Spriteに付与するタグ。
	/// StatusUpgradeComponent::IsConfirming中のみ表示する</summary>
	struct StatusUpgradeConfirmWindowUiTag
	{
	};

	/// <summary>フィードバックメッセージ表示テキストに付与するタグ</summary>
	struct StatusUpgradeMessageUiTag
	{
	};

	/// <summary>
	/// 画面下部の操作案内(2行)を識別するタグ。「Selectって何ボタン？」とならないよう、
	/// 最後に使われた入力デバイス(キーボード/マウス or パッド)に応じてボタン表示名
	/// (Space/Aボタン等)が変わるため、内容を毎フレーム更新する。
	/// </summary>
	struct StatusUpgradeGuideUiTag
	{
		/// <summary>0:移動/強化/最大強化の案内行、1:全リセット/戻るの案内行</summary>
		int LineIndex = 0;
	};
}

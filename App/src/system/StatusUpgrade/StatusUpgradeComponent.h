#pragma once

#include<string>

namespace ecs
{
	///<summary>
	///強化確認ダイアログのはい/いいえの選択肢
	///</summary>
	enum class eStatusUpgradeConfirmOption
	{
		Yes,
		No,
	};

	///<summary>
	///ステータス強化画面のカーソル位置・確認ダイアログ・フィードバックメッセージの状態を保持するコンポーネント。選択肢はdata::eStatUpgradeTypeと対応する固定12件
	///</summary>
	struct StatusUpgradeComponent
	{
		static constexpr int kOptionCount = 12;

		///<summary>
		///現在カーソルが当たっている選択肢、0からkOptionCount-1でdata::eStatUpgradeTypeと対応
		///</summary>
		int SelectedIndex = 0;

		///<summary>
		///強化しますかの確認ダイアログを表示中か。true中はMenuUp/MenuDownでのカーソル移動とCancelでのHubSceneへの遷移を止め、MenuLeft/MenuRightでのはい/いいえ選択とSelectでの決定操作だけを受け付ける
		///</summary>
		bool IsConfirming = false;

		///<summary>
		///確認ダイアログ内で現在選択されている項目。ダイアログを開くたびに誤操作防止のためNoへ戻す
		///</summary>
		eStatusUpgradeConfirmOption ConfirmSelectedOption = eStatusUpgradeConfirmOption::No;

		///<summary>
		///フィードバックメッセージの残り表示時間、秒。0以下なら非表示
		///</summary>
		float MessageTimer = 0.0f;

		///<summary>
		///表示中のフィードバックメッセージ本文
		///</summary>
		std::wstring Message;
	};

	///<summary>
	///ステータス強化画面の1カードを構成する要素の種類。レイアウトは名前が上、アイコンが中、強化状態が下の縦積み
	///</summary>
	enum class eStatusUpgradeCardElement
	{
		NameText,  // ステータス名(アイコンの上)
		Icon,      // アイコン画像本体
		StateText, // Lv./コスト等の強化状態(アイコンの下)
	};

	///<summary>
	///ステータス強化画面の1カードを識別するタグ。NameText/Icon/StateTextの3エンティティが同じOptionIndexを共有する
	///</summary>
	struct StatusUpgradeCardUiTag
	{
		///<summary>
		///対応する選択肢インデックス、0からStatusUpgradeComponent::kOptionCount-1でdata::eStatUpgradeTypeと対応
		///</summary>
		int OptionIndex = 0;
		eStatusUpgradeCardElement Element = eStatusUpgradeCardElement::Icon;

		///<summary>
		///Element==NameText/StateTextの場合のみ使用。テキストの水平中央揃えの基準X座標、不変。TextComponent::Xは毎フレーム書き換えられるため、基準座標を別途保持しないと位置がズレ続けるバグになる
		///</summary>
		float CenterX = 0.0f;
	};

	///<summary>
	///所持ゴールド表示テキストに付与するタグ
	///</summary>
	struct StatusUpgradeGoldUiTag
	{
	};

	///<summary>
	///強化しますかの確認ダイアログ表示テキストに付与するタグ
	///</summary>
	struct StatusUpgradeConfirmUiTag
	{
	};

	///<summary>
	///確認ダイアログのはい/いいえの選択肢テキストを識別するタグ
	///</summary>
	struct StatusUpgradeConfirmOptionUiTag
	{
		eStatusUpgradeConfirmOption Option = eStatusUpgradeConfirmOption::No;
	};

	///<summary>
	///確認ダイアログの操作案内テキストに付与するタグ
	///</summary>
	struct StatusUpgradeConfirmGuideUiTag
	{
	};

	///<summary>
	///確認ダイアログの背景、黒半透明で画面中心のSpriteに付与するタグ。StatusUpgradeComponent::IsConfirming中のみ表示する
	///</summary>
	struct StatusUpgradeConfirmWindowUiTag
	{
	};

	///<summary>
	///フィードバックメッセージ表示テキストに付与するタグ
	///</summary>
	struct StatusUpgradeMessageUiTag
	{
	};

	///<summary>
	///画面下部の操作案内、2行を識別するタグ。最後に使われた入力デバイスに応じてボタン表示名が変わるため、内容を毎フレーム更新する
	///</summary>
	struct StatusUpgradeGuideUiTag
	{
		///<summary>
		///0が移動/強化/最大強化の案内行、1が全リセット/戻るの案内行
		///</summary>
		int LineIndex = 0;
	};
}

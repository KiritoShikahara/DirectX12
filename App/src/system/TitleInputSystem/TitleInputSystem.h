#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	///<summary>
	///タイトル画面の操作案内テキストを識別
	///</summary>
	struct TitleGuideUiTag
	{
	};

	///<summary>
	///タイトル画面の操作案内を表示する背景パネルを識別。表示デバイスにより文言の長さが変わるため毎フレーム幅を再計算する
	///</summary>
	struct TitleGuidePanelUiTag
	{
	};

	///<summary>
	///ゲーム終了確認ダイアログのはい/いいえの選択肢
	///</summary>
	enum class eTitleConfirmOption
	{
		Yes,
		No,
	};

	///<summary>
	///タイトル画面のゲーム終了確認ダイアログの表示状態を保持する
	///</summary>
	struct TitleComponent
	{
		///<summary>
		///ゲーム終了確認ダイアログを表示中か。true中はMenuLeft/MenuRightでのはい/いいえ選択とSelectでの決定、Cancelでの取り消しのみ受け付ける
		///</summary>
		bool IsConfirmingExit = false;

		///<summary>
		///確認ダイアログ内で現在選択されている項目。ダイアログを開くたびに誤操作防止のためNoへ戻す
		///</summary>
		eTitleConfirmOption ConfirmSelectedOption = eTitleConfirmOption::No;
	};

	///<summary>
	///ゲーム終了確認ダイアログの背景、黒半透明のSpriteに付与するタグ。TitleComponent::IsConfirmingExit中のみ表示する
	///</summary>
	struct TitleExitConfirmWindowUiTag
	{
	};

	///<summary>
	///ゲーム終了確認ダイアログの見出しテキストに付与するタグ
	///</summary>
	struct TitleExitConfirmQuestionUiTag
	{
	};

	///<summary>
	///ゲーム終了確認ダイアログのはい/いいえの選択肢テキストを識別するタグ
	///</summary>
	struct TitleExitConfirmOptionUiTag
	{
		eTitleConfirmOption Option = eTitleConfirmOption::No;
	};

	///<summary>
	///ゲーム終了確認ダイアログの操作案内テキストに付与するタグ
	///</summary>
	struct TitleExitConfirmGuideUiTag
	{
	};
}

namespace sys
{
	///<summary>
	///タイトル画面の入力を処理するシステム
	///</summary>
	class TitleInputSystem : public ::ecs::IUserSystem
	{
	public:
		///<summary>
		///タイトル画面の入力を更新
		///</summary>
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}
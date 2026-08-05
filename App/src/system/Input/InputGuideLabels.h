#pragma once

#include<system/Input/InputManager.h>

namespace ecs::inputguide
{
	///<summary>
	///メニュー画面のカーソル移動、MenuUp/Down/Left/Right。KBはWASD/矢印キー、Padは十字ボタン
	///</summary>
	inline const wchar_t* GetMenuMoveLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"十字ボタン" : L"WASD/矢印キー";
	}

	///<summary>
	///ゲームプレイ中の移動。KBはWASD、Padは左スティックのアナログ。メニューの十字ボタンとは別物なので混同しないこと
	///</summary>
	inline const wchar_t* GetGameplayMoveLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"左スティック" : L"WASD";
	}

	///<summary>
	///照準/視点。KBはマウス、Padは右スティック
	///</summary>
	inline const wchar_t* GetAimLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"右スティック" : L"マウス";
	}

	///<summary>
	///決定/1件選択、Select。KBはSpaceまたは左クリック、PadはAボタン
	///</summary>
	inline const wchar_t* GetSelectLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Aボタン" : L"Space";
	}

	///<summary>
	///一括操作/確認ダイアログを開く、SelectAll。KBはEnter、PadはYボタン
	///</summary>
	inline const wchar_t* GetSelectAllLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Yボタン" : L"Enter";
	}

	///<summary>
	///削除/全リセット、Delete。KBはDelete、PadはXボタン
	///</summary>
	inline const wchar_t* GetDeleteLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Xボタン" : L"Delete";
	}

	///<summary>
	///キャンセル/戻る、Cancel。KBはEsc、PadはBボタン
	///</summary>
	inline const wchar_t* GetCancelLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Bボタン" : L"Esc";
	}

	///<summary>
	///メイン攻撃、Attack。KBは左クリック、PadはR2
	///</summary>
	inline const wchar_t* GetAttackLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"R2" : L"左クリック";
	}

	///<summary>
	///サブ攻撃、Attack2。KBは右クリック、PadはL2
	///</summary>
	inline const wchar_t* GetAttack2Label(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"L2" : L"右クリック";
	}

	///<summary>
	///必殺技、Ultimate。KBはQ、PadはR1
	///</summary>
	inline const wchar_t* GetUltimateLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"R1" : L"Q";
	}

	///<summary>
	///フリッカーストライク、FlickerStrike。KBはマウスホイール押し込み、PadはYボタン
	///</summary>
	inline const wchar_t* GetFlickerStrikeLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Yボタン" : L"ホイールクリック";
	}

	///<summary>
	///ポーズ/オプションメニューを開く、Option。KBはEsc、PadはMenuボタン
	///</summary>
	inline const wchar_t* GetOptionLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Menuボタン" : L"Esc";
	}
}

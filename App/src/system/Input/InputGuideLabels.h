#pragma once

#include<system/Input/InputManager.h>

namespace ecs::inputguide
{
	/// <summary>
	/// 各画面の操作案内に表示するボタン名を、最後に使われた入力デバイス
	/// (InputManager::GetLastInputDevice)に応じて切り替えるための共通ヘルパー。
	/// 「Selectって何ボタン？」とならないよう、キーボード/マウス操作時は物理キー名、
	/// パッド操作時はパッドボタン名を返す。実際のバインディングは
	/// CoreEngine/src/system/Input/InputManager.cppのAddAction呼び出しと必ず一致させること。
	/// 元はStatusUpgradeInputSystem.cppにローカルで持っていたが、Title/Hub/Menu/Game
	/// (Optionsメニュー)等、複数画面から使うため共有ヘッダーへ切り出した。
	/// </summary>

	/// <summary>メニュー画面のカーソル移動(MenuUp/Down/Left/Right)。
	/// KB: WASD/矢印キー、Pad: 十字ボタン(D-Pad)</summary>
	inline const wchar_t* GetMenuMoveLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"十字ボタン" : L"WASD/矢印キー";
	}

	/// <summary>ゲームプレイ中の移動(InputManager::GetMoveAxis)。
	/// KB: WASD、Pad: 左スティック(アナログ)。メニューの十字ボタンとは別物なので混同しないこと</summary>
	inline const wchar_t* GetGameplayMoveLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"左スティック" : L"WASD";
	}

	/// <summary>照準/視点(InputManager::GetLookAxis)。KB: マウス、Pad: 右スティック</summary>
	inline const wchar_t* GetAimLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"右スティック" : L"マウス";
	}

	/// <summary>決定/1件選択("Select")。KB: Space(マウス左クリックも可)、Pad: Aボタン</summary>
	inline const wchar_t* GetSelectLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Aボタン" : L"Space";
	}

	/// <summary>一括操作/確認ダイアログを開く("SelectAll")。KB: Enter、Pad: Yボタン</summary>
	inline const wchar_t* GetSelectAllLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Yボタン" : L"Enter";
	}

	/// <summary>削除/全リセット("Delete")。KB: Delete、Pad: Xボタン</summary>
	inline const wchar_t* GetDeleteLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Xボタン" : L"Delete";
	}

	/// <summary>キャンセル/戻る("Cancel")。KB: Esc、Pad: Bボタン</summary>
	inline const wchar_t* GetCancelLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Bボタン" : L"Esc";
	}

	/// <summary>メイン攻撃("Attack")。KB: 左クリック、Pad: R2</summary>
	inline const wchar_t* GetAttackLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"R2" : L"左クリック";
	}

	/// <summary>サブ攻撃("Attack2")。KB: 右クリック、Pad: L2</summary>
	inline const wchar_t* GetAttack2Label(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"L2" : L"右クリック";
	}

	/// <summary>必殺技("Ultimate")。KB: Q、Pad: R1</summary>
	inline const wchar_t* GetUltimateLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"R1" : L"Q";
	}

	/// <summary>フリッカーストライク("FlickerStrike")。KB: R、Pad: Yボタン</summary>
	inline const wchar_t* GetFlickerStrikeLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Yボタン" : L"R";
	}

	/// <summary>ポーズ/オプションメニューを開く("Option")。KB: Esc、Pad: Menuボタン</summary>
	inline const wchar_t* GetOptionLabel(::sys::eInputDevice device)
	{
		return device == ::sys::eInputDevice::Pad ? L"Menuボタン" : L"Esc";
	}
}

#pragma once

namespace ecs
{
	enum class eWeaponType;
}

namespace ecs::weaponutil
{
	///<summary>
	///武器種別に対応する表示アイコンのパスを返す。専用アイコン未作成の種別はloading.pngを返す
	///</summary>
	const char* GetWeaponIconPath(eWeaponType type);

	///<summary>
	///武器種別に対応する「発動操作」アイコン(キーボード+マウス用)のパスを返す。
	///eWeaponControl::Auto(自動発動)でプレイヤー操作を必要としない武器はnullptrを返す
	///</summary>
	const char* GetWeaponControlIconPath(eWeaponType type);
}

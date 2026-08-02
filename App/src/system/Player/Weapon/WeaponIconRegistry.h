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
}

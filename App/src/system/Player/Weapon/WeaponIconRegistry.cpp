#include "apppch.h"
#include "WeaponIconRegistry.h"

#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>

namespace ecs::weaponutil
{
	const char* GetWeaponIconPath(eWeaponType type)
	{
		// 未作成アイコンの代用、後で差し替える前提のプレースホルダ
		constexpr const char* kFallbackIcon = "Assets/Icon/loading.png";

		switch (type)
		{
		case eWeaponType::SingleShot:    return "Assets/Icon/fire.png";        // FireBolt
		case eWeaponType::SelfDefense:   return "Assets/Icon/frozen.png";      // FrostOrb
		case eWeaponType::Nova:          return "Assets/Icon/area_attack.png"; // 範囲攻撃系の暫定割当
		case eWeaponType::Chain:         return "Assets/Icon/chain.png";
		case eWeaponType::VoidBeam:      return "Assets/Icon/beam.png";
		case eWeaponType::BoneSpear:     return "Assets/Icon/bone_spear.png";
		case eWeaponType::FlickerStrike: return "Assets/Icon/flicker.png";

		case eWeaponType::AreaAttack:    // IceSpike: 専用アイコン未作成
		case eWeaponType::Homing:        // Homing Missile: 専用アイコン未作成
		case eWeaponType::Meteor:        // area_attack.pngはNovaで使用済みのため代用のまま区別する
		case eWeaponType::Cleave:        // 専用アイコン未作成
		case eWeaponType::Ricochet:      // 専用アイコン未作成
		default:
			return kFallbackIcon;
		}
	}

	const char* GetWeaponControlIconPath(eWeaponType type)
	{
		// eWeaponControl::Manualな武器種別は、各WeaponSystem(SingleShot/AreaAttack/FlickerStrike)側で
		// 発動に使う入力アクションが型ごとに固定されているため、ここでも型で対応アイコンを固定する
		switch (type)
		{
		case eWeaponType::SingleShot:     return "Assets/Icon/mouse_left.png";   // Attack(左クリック)で発動
		case eWeaponType::AreaAttack:     return "Assets/Icon/mouse_right.png";  // Attack2(右クリック)で発動
		case eWeaponType::FlickerStrike:  return "Assets/Icon/mouse_middle.png"; // FlickerStrike(マウスホイール押し込み)で発動
		default:                          return nullptr; // Auto(自動発動)の武器は操作方法アイコン無し
		}
	}
}

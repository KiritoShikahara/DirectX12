#pragma once

#include<string>
#include<vector>
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>

namespace ecs
{
	/// <summary>
	/// パークが実際に適用する効果の種別。
	/// </summary>
	enum class ePerkEffectType
	{
		MaxHpUp,       // PlayerStatusComponent.Modifier.MulMaxHp += Magnitude（増加分だけ現在HPも回復）
		CooldownDown,  // PlayerStatusComponent.Modifier.MulCooldownRate += Magnitude（Magnitudeは負値）
		MoveSpeedUp,   // PlayerStatusComponent.Modifier.MulMoveSpeed += Magnitude
		WeaponLevelUp, // 所持武器のうちMaxLevel未満のものを1つ選びLevelを+1する（Magnitude未使用）
		AcquireWeapon, // 所持していない指定の武器(AcquireWeaponType/AcquireWeaponId)を1つ取得する（Magnitude未使用）
	};

	/// <summary>
	/// パーク1件分の定義（表示名＋効果）。
	/// </summary>
	struct PerkDefinition
	{
		ePerkEffectType Type;
		std::wstring    Name;          // UI表示名
		float           Magnitude = 0.0f; // 効果量。Modifierへの加算値（WeaponLevelUp/AcquireWeaponでは未使用）

		// AcquireWeapon専用: 取得する武器の種別とID（他の効果種別では未使用）
		eWeaponType AcquireWeaponType = eWeaponType::SingleShot;
		int         AcquireWeaponId = 0;
	};

	/// <summary>
	/// パーク候補プール（個人開発プロトタイプの暫定ラインナップ。
	/// 具体的な最終ラインナップは未確定＝GAME_DESIGN.md参照、後で調整・追加する前提）。
	///
	/// PlayerStatusComponent::Current.AtkPowerは各武器のダメージ計算(ecs::combatutil::
	/// GetAtkPowerMultiplier経由)に、Defenseは被ダメージ軽減式(PlayerContactDamageSystemの
	/// 半減点方式)に接続済み。AtkPower/Defense系のパークもプールに追加可能な状態になったが、
	/// 現状はまだ未追加（次の調整候補）。
	/// </summary>
	inline const std::vector<PerkDefinition>& GetPerkPool()
	{
		static const std::vector<PerkDefinition> pool =
		{
			{ ePerkEffectType::MaxHpUp,       L"最大HP + 15%",    0.15f },
			{ ePerkEffectType::CooldownDown,  L"攻撃間隔 - 10%",  -0.10f },
			{ ePerkEffectType::MoveSpeedUp,   L"移動速度 + 10%",  0.10f },
			{ ePerkEffectType::WeaponLevelUp, L"武器レベルアップ", 0.0f },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Nova",    0.0f, eWeaponType::Nova, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Homing Missile", 0.0f, eWeaponType::Homing, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Chain Lightning", 0.0f, eWeaponType::Chain, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Meteor",  0.0f, eWeaponType::Meteor, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Void Beam", 0.0f, eWeaponType::VoidBeam, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Bone Spear", 0.0f, eWeaponType::BoneSpear, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Cleave", 0.0f, eWeaponType::Cleave, 0 },
		};
		return pool;
	}
}

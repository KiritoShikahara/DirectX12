#pragma once

#include<string>
#include<vector>
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<Data/Perk/PerkData.h>

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
		AtkPowerUp,    // PlayerStatusComponent.Modifier.MulAtkPower += Magnitude
		DefenseUp,     // PlayerStatusComponent.Modifier.MulDefense += Magnitude
		AttackCountUp, // PlayerStatusComponent.Modifier.MulAttackCount += Magnitude（1回の発動で放つ攻撃回数の倍率）
		WeaponLevelUp, // 所持武器のうちMaxLevel未満のものを1つ選びLevelを+1する（Magnitude未使用）
		AcquireWeapon, // 所持していない指定の武器(AcquireWeaponType/AcquireWeaponId)を1つ取得する（Magnitude未使用）

		// 以下は既存のdb.db/PerkData.csvのId(0-7)を変えないよう末尾に追加すること
		HealHp,           // PlayerStatusComponent.CurrentHp += Current.MaxHp × Magnitude（即時回復、上限MaxHp）
		ExperienceGainUp, // PlayerLevelComponent.MulExperienceGain += Magnitude（経験値獲得量の倍率）

		// --- 複合効果・トレードオフ系（以降も末尾へ追加すること） ---

		/// <summary>
		/// 全ステータス(最大HP/移動速度/攻撃力/防御力/攻撃間隔)をMagnitudeだけまとめて強化する。
		/// 1つの効果が突出しない代わりに全体が底上げされる、腐りにくい選択肢。
		/// </summary>
		AllStatsUp,

		/// <summary>
		/// HPが0になったとき1回だけ全回復して死亡を取り消す
		/// (PlayerStatusComponent::ReviveCountを+1する。Magnitude未使用)。
		/// </summary>
		Revive,

		/// <summary>
		/// 経験値獲得量を下げる代わりに攻撃力を大きく上げる。
		/// Magnitudeを攻撃力の増加量、TradeoffMagnitudeを経験値の減少量として使う。
		/// </summary>
		GlassCannon,

		/// <summary>
		/// 最大HPを下げる代わりに移動速度と攻撃間隔を大きく改善する。
		/// Magnitudeを速度系の改善量、TradeoffMagnitudeを最大HPの減少量として使う。
		/// </summary>
		Berserk,

		/// <summary>
		/// 防御を捨てて攻撃回数を増やす。
		/// Magnitudeを攻撃回数の増加量、TradeoffMagnitudeを防御力の減少量として使う。
		/// </summary>
		Reckless,
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

		/// <summary>
		/// トレードオフ系(GlassCannon/Berserk/Reckless)で「代償」として下がる側の量。
		/// メリット側はMagnitudeを使う。他の効果種別では未使用。
		/// </summary>
		float TradeoffMagnitude = 0.0f;
	};

	/// <summary>
	/// パーク候補プール（個人開発プロトタイプの暫定ラインナップ。
	/// 具体的な最終ラインナップは未確定＝GAME_DESIGN.md参照、後で調整・追加する前提）。
	///
	/// PlayerStatusComponent::Current.AtkPowerは各武器のダメージ計算(ecs::combatutil::
	/// GetAtkPowerMultiplier経由)に、Defenseは被ダメージ軽減式(PlayerContactDamageSystemの
	/// 半減点方式)に接続済み。AttackCountはecs::combatutil::GetAttackCount経由で、
	/// 発動が明確な武器(Orbit/FlickerStrikeを除く9種)がFire/Pulse/Swing/Zapをその回数だけ
	/// 繰り返す形で反映される。HealHpは即時回復(Modifierを変更しない一回性の効果、
	/// WeaponLevelUpと同じ扱い)。ExperienceGainUpはPlayerLevelComponentの経験値獲得倍率。
	/// </summary>
	inline const std::vector<PerkDefinition>& GetPerkPool()
	{
		static const std::vector<PerkDefinition> pool =
		{
			{ ePerkEffectType::MaxHpUp,       L"最大HP + 15%",    0.15f },
			{ ePerkEffectType::CooldownDown,  L"攻撃間隔 - 10%",  -0.10f },
			{ ePerkEffectType::MoveSpeedUp,   L"移動速度 + 10%",  0.10f },
			{ ePerkEffectType::AtkPowerUp,    L"攻撃力 + 10%",    0.10f },
			{ ePerkEffectType::DefenseUp,     L"防御力 + 15%",    0.15f },
			{ ePerkEffectType::AttackCountUp, L"同時攻撃数 + 100%", 1.0f },
			{ ePerkEffectType::WeaponLevelUp, L"武器レベルアップ", 0.0f },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Nova",    0.0f, eWeaponType::Nova, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Homing Missile", 0.0f, eWeaponType::Homing, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Chain Lightning", 0.0f, eWeaponType::Chain, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Meteor",  0.0f, eWeaponType::Meteor, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Void Beam", 0.0f, eWeaponType::VoidBeam, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Bone Spear", 0.0f, eWeaponType::BoneSpear, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Cleave", 0.0f, eWeaponType::Cleave, 0 },
			{ ePerkEffectType::AcquireWeapon, L"新武器: Flicker Strike", 0.0f, eWeaponType::FlickerStrike, 0 },
			{ ePerkEffectType::HealHp,           L"HP回復 30%",         0.30f },
			{ ePerkEffectType::ExperienceGainUp, L"経験値獲得量 + 15%", 0.15f },

			// --- 全ステータス強化(伸び幅は控えめだが腐りにくい) ---
			{ ePerkEffectType::AllStatsUp, L"全ステータス + 3%", 0.03f },
			{ ePerkEffectType::AllStatsUp, L"全ステータス + 5%", 0.05f },

			// --- 一回性の保険 ---
			{ ePerkEffectType::Revive, L"復活 (1回だけ死亡を無効化)", 0.0f },

			// --- トレードオフ(Magnitude=メリット, TradeoffMagnitude=デメリット) ---
			// ハイリスク・ハイリターンの選択肢。数値は個人開発プロトタイプの暫定値で、
			// プレイ感触に応じて調整すること
			{ ePerkEffectType::GlassCannon, L"攻撃力 + 50% / 経験値 - 20%",
				0.50f, eWeaponType::SingleShot, 0, 0.20f },
			{ ePerkEffectType::Berserk,     L"移動速度・攻撃間隔 + 25% / 最大HP - 20%",
				0.25f, eWeaponType::SingleShot, 0, 0.20f },
			{ ePerkEffectType::Reckless,    L"同時攻撃数 + 100% / 防御力 - 50%",
				1.00f, eWeaponType::SingleShot, 0, 0.50f },
		};
		return pool;
	}

	/// <summary>
	/// 指定のパーク種別を選択できる最大回数(data::PerkData::MaxLevel、CSV/DB)を求める。
	/// データが未登録の場合は実質無制限(選択を誤ってブロックしないためのフォールバック)。
	/// </summary>
	inline int GetPerkMaxLevel(ePerkEffectType type)
	{
		constexpr int kFallbackMaxLevel = 99;
		const auto* perkData = DATA_MGR(data::PerkData).GetById(static_cast<int>(type));
		return perkData != nullptr ? perkData->MaxLevel : kFallbackMaxLevel;
	}

	/// <summary>
	/// 指定のパーク種別の抽選重み(data::PerkData::Weight、CSV/DB)を求める。
	/// データが未登録の場合は等倍(1.0)として扱う。
	/// </summary>
	inline float GetPerkWeight(ePerkEffectType type)
	{
		constexpr float kFallbackWeight = 1.0f;
		const auto* perkData = DATA_MGR(data::PerkData).GetById(static_cast<int>(type));
		return perkData != nullptr ? std::max(0.0f, perkData->Weight) : kFallbackWeight;
	}
}

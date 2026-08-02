#pragma once

#include <Data/Storage/Reflection.h>
#include <Data/Storage/Registry/ConfigRegistry.h>

namespace data
{
	/// <summary>プレイヤーの永続的な進行状況（所持ゴールド・ステータス恒久強化レベル）</summary>
	struct PlayerSaveData
	{
		/// <summary>所持ゴールド</summary>
		int Gold = 0;

		// 各ステータスの強化レベル(0始まり)。上限はStatUpgradeData::MaxLevel(CSV/DB)で管理する。
		int MaxHpLevel = 0;
		int AtkPowerLevel = 0;
		int DefenseLevel = 0;
		int CooldownRateLevel = 0;
		int MoveSpeedLevel = 0;
		int GoldGainRateLevel = 0;
		int HpRegenLevel = 0;
		int ExperienceGainRateLevel = 0;
		int AttackCountLevel = 0;
		int ReviveLevel = 0;
		int PostHitInvincibilityLevel = 0;
		int PerkChoiceCountLevel = 0;

		REFLECT_BEGIN(PlayerSaveData, "player_save")
			REFLECT_FIELD_INT(Gold)
			REFLECT_FIELD_INT(MaxHpLevel)
			REFLECT_FIELD_INT(AtkPowerLevel)
			REFLECT_FIELD_INT(DefenseLevel)
			REFLECT_FIELD_INT(CooldownRateLevel)
			REFLECT_FIELD_INT(MoveSpeedLevel)
			REFLECT_FIELD_INT(GoldGainRateLevel)
			REFLECT_FIELD_INT(HpRegenLevel)
			REFLECT_FIELD_INT(ExperienceGainRateLevel)
			REFLECT_FIELD_INT(AttackCountLevel)
			REFLECT_FIELD_INT(ReviveLevel)
			REFLECT_FIELD_INT(PostHitInvincibilityLevel)
			REFLECT_FIELD_INT(PerkChoiceCountLevel)
			REFLECT_END()
	};

	/// <summary>PlayerSaveDataをConfigRegistryへ未登録なら登録し、ディスクから読み込む</summary>
	inline void EnsurePlayerSaveDataLoaded()
	{
		auto& configReg = ::data::ConfigRegistry::Get();
		if (configReg.IsRegistered<PlayerSaveData>()) return;

		configReg.Register<PlayerSaveData>("Assets/Bin/Save/player_save.json");
		configReg.GetManager<PlayerSaveData>().Load();
	}
}

REFLECT_REGISTER(data::PlayerSaveData);
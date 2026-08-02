#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
	/// <summary>StatUpgradeData::Idと対応する強化対象ステータスの種別</summary>
	enum class eStatUpgradeType : int
	{
		MaxHp = 0,
		AtkPower = 1,
		Defense = 2,
		CooldownRate = 3,
		MoveSpeed = 4,
		GoldGainRate = 5,       
		HpRegen = 6,            
		ExperienceGainRate = 7, 

		AttackCount = 8,         
		Revive = 9,               
		PostHitInvincibility = 10,
		PerkChoiceCount = 11,
	};

	/// <summary>ステータス恒久強化のバランス調整データ(CSV/DB)</summary>
	struct StatUpgradeData
	{
		int Id = 0;
		std::string Name;

		float BaseCost = 100.0f;
		float CostGrowthPerLevel = 50.0f;
		float ValuePerLevel = 1.0f;
		int MaxLevel = 10;

		REFLECT_BEGIN(StatUpgradeData, "stat_upgrade_data")
			REFLECT_FIELD_ID(Id)
			REFLECT_FIELD_STR(Name)
			REFLECT_FIELD_FLOAT(BaseCost)
			REFLECT_FIELD_FLOAT(CostGrowthPerLevel)
			REFLECT_FIELD_FLOAT(ValuePerLevel)
			REFLECT_FIELD_INT(MaxLevel)
			REFLECT_END()
	};
}

REFLECT_REGISTER(data::StatUpgradeData);
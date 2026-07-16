#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
	/// <summary>
	/// StatUpgradeData::Idと対応する強化対象ステータスの種別。
	/// PlayerSaveDataの各レベルフィールドと1対1で対応する。
	/// </summary>
	enum class eStatUpgradeType : int
	{
		MaxHp = 0,
		AtkPower = 1,
		Defense = 2,
		CooldownRate = 3,
	};

	/// <summary>
	/// ステータス恒久強化のバランス調整データ(CSV/DB)。対象ステータス1件につき1行。
	/// Id(eStatUpgradeTypeと対応)・表示名・コスト曲線・効果量・レベル上限を持つ。
	/// GUIからの調整はDataInspector経由(専用デバッグパネルは現状未整備、必要になり次第追加)。
	///
	/// コスト = BaseCost + CostGrowthPerLevel × 現在レベル（レベルが上がるごとに高くなる）。
	/// ValuePerLevelはPlayerStatusComponent::Baseへ加算する量
	/// （CooldownRateは下げたいため負値を設定する想定）。
	/// </summary>
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

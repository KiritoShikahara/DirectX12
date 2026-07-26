#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
	/// <summary>
	/// パーク種別ごとに選択できる最大回数(MaxLevel)を管理するバランス調整データ(CSV/DB)。
	/// Id = ecs::ePerkEffectType の値と対応する(PerkDefinition.h参照)。
	/// </summary>
	struct PerkData
	{
		int         Id = 0;        // ecs::ePerkEffectTypeの値
		std::string Name;          // デバッグ表示用(パーク種別名)
		int         MaxLevel = 99; // このパーク種別を選択できる最大回数

		/// <summary>
		/// 抽選の重み。大きいほど選択肢に出やすい(相対値、合計が1である必要はない)。
		/// 「その他」枠(3〜5番目)の抽選にのみ影響する(PerkSelectSystem::EnterPerkSelect参照)。
		/// </summary>
		float Weight = 1.0f;

		REFLECT_BEGIN(PerkData, "perks")
			REFLECT_FIELD_ID(Id)
			REFLECT_FIELD_STR(Name)
			REFLECT_FIELD_INT(MaxLevel)
			REFLECT_FIELD_FLOAT(Weight)
		REFLECT_END()
	};
}

REFLECT_REGISTER(data::PerkData);

#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
	/// <summary>
	/// パーク種別ごとに選択できる最大回数(MaxLevel)を管理するバランス調整データ(CSV/DB)。
	/// Id = ecs::ePerkEffectType の値と対応する(PerkDefinition.h参照)。
	///
	/// PlayerPerkLevelComponentが実際の選択回数(GetPerkPool()のインデックスごと)を記録し、
	/// PerkSelectSystemがそのパーク種別のMaxLevelに達した候補を提示対象から除外する。
	/// WeaponLevelUp/AcquireWeaponは既に別系統の判定(所持武器のレベル・所持状況)で
	/// 実質的に上限管理されているため、ここでは大きめの値(実質無制限)を設定している。
	/// </summary>
	struct PerkData
	{
		int         Id = 0;        // ecs::ePerkEffectTypeの値
		std::string Name;          // デバッグ表示用(パーク種別名)
		int         MaxLevel = 99; // このパーク種別を選択できる最大回数

		REFLECT_BEGIN(PerkData, "perks")
			REFLECT_FIELD_ID(Id)
			REFLECT_FIELD_STR(Name)
			REFLECT_FIELD_INT(MaxLevel)
		REFLECT_END()
	};
}

REFLECT_REGISTER(data::PerkData);

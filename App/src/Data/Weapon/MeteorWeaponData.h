#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
	struct MeteorWeaponData
	{
		int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
		std::string Name;                    // 表示・デバッグ用

		float       FireInterval = 4.0f;     // 発動間隔(秒)
		float       Damage = 20.0f;          // 火力
		float       Radius = 12.0f;          // 半径(m)
		float       HitRadiusMultiplier = 1.5f; // 判定拡大倍率
		float       SearchRadius = 60.0f;    // 索敵範囲(m)
		int         MeteorCount = 3;         // 隕石数
		float       HeightOffset = 0.0f;     // 高さオフセット(m)
		std::string EffectIds;               // 着弾エフェクト素材ID

		REFLECT_BEGIN(MeteorWeaponData, "meteor_weapons")
			REFLECT_FIELD_ID(Id)
			REFLECT_FIELD_STR(Name)
			REFLECT_FIELD_FLOAT(FireInterval)
			REFLECT_FIELD_FLOAT(Damage)
			REFLECT_FIELD_FLOAT(Radius)
			REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
			REFLECT_FIELD_FLOAT(SearchRadius)
			REFLECT_FIELD_INT(MeteorCount)
			REFLECT_FIELD_FLOAT(HeightOffset)
			REFLECT_FIELD_STR(EffectIds)
			REFLECT_END()
	};
}

REFLECT_REGISTER(data::MeteorWeaponData);
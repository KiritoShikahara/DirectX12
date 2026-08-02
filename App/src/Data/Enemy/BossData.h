#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
	/// <summary>ボース階級ごとの強化倍率マスタ(CSV/DB)。Id=0:小ボース, 1:中ボース, 2:最強ボース</summary>
	struct BossData
	{
		int         Id = 0;              // ボース階級(eBossTierのint値と対応、主キー)
		std::string Name;                // 表示・デバッグ用

		float       HpMultiplier = 1.0f;   // 最大HPの追加倍率
		float       AtkMultiplier = 1.0f;  // 攻撃力の追加倍率
		float       ScaleMultiplier = 1.0f;// 見た目・コライダーの拡大倍率
		float       ExpMultiplier = 1.0f;  // 撃破時経験値の追加倍率
		float       GoldMultiplier = 1.0f; // 撃破時ゴールドの追加倍率

		REFLECT_BEGIN(BossData, "boss_tiers")
			REFLECT_FIELD_ID(Id)
			REFLECT_FIELD_STR(Name)
			REFLECT_FIELD_FLOAT(HpMultiplier)
			REFLECT_FIELD_FLOAT(AtkMultiplier)
			REFLECT_FIELD_FLOAT(ScaleMultiplier)
			REFLECT_FIELD_FLOAT(ExpMultiplier)
			REFLECT_FIELD_FLOAT(GoldMultiplier)
			REFLECT_END()
	};
}

REFLECT_REGISTER(data::BossData);
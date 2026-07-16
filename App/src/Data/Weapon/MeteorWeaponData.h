#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
	/// <summary>
	/// Meteor型武器のマスタデータ（CSV/DB）。
	/// WeaponComponent::WeaponID と対応する。
	/// ダメージ・範囲半径は Base + PerLevel * (Level - 1) の線形成長とする
	/// （他のWeaponDataと同じ方式）。
	/// CSV ヘッダー名は各フィールド名と完全一致すること。
	///
	/// 発動トリガーが無く、FireInterval秒ごとにSearchRadius内の敵からランダムに
	/// 最大MeteorCount体を選び、それぞれの頭上へ隕石を落として範囲ダメージを与える
	/// （MeteorWeaponSystemが担当）。狙い・移動を必要としない完全自動の武器
	/// （パーク選択でのみ取得可能）。
	/// </summary>
	struct MeteorWeaponData
	{
		int         Id = 0;                  // 武器ID（主キー。WeaponComponent::WeaponID と対応）
		std::string Name;                    // 表示・デバッグ用

		float       FireInterval = 4.0f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

		float       BaseDamage = 20.0f;      // Lv1火力
		float       DamagePerLevel = 5.0f;   // レベル毎の火力増加量

		float       BaseRadius = 12.0f;      // Lv1のエフェクト見た目基準半径(m)
		float       RadiusPerLevel = 1.5f;   // レベル毎の見た目基準半径増加量

		float       HitRadiusMultiplier = 1.5f; // 実際の当たり判定半径 = 上記(BaseRadius系)×この値

		float       SearchRadius = 60.0f;    // この範囲内の敵を対象候補にする
		int         MeteorCount = 3;         // 1回の発動で落とす隕石の数(=対象数の上限)

		float       HeightOffset = 0.0f;     // 着弾位置のY座標オフセット(m)

		std::string EffectPath;              // 着弾エフェクト(.efk、';'区切りで複数指定可)

		REFLECT_BEGIN(MeteorWeaponData, "meteor_weapons")
			REFLECT_FIELD_ID(Id)
			REFLECT_FIELD_STR(Name)
			REFLECT_FIELD_FLOAT(FireInterval)
			REFLECT_FIELD_FLOAT(BaseDamage)
			REFLECT_FIELD_FLOAT(DamagePerLevel)
			REFLECT_FIELD_FLOAT(BaseRadius)
			REFLECT_FIELD_FLOAT(RadiusPerLevel)
			REFLECT_FIELD_FLOAT(HitRadiusMultiplier)
			REFLECT_FIELD_FLOAT(SearchRadius)
			REFLECT_FIELD_INT(MeteorCount)
			REFLECT_FIELD_FLOAT(HeightOffset)
			REFLECT_FIELD_STR(EffectPath)
		REFLECT_END()
	};
}

REFLECT_REGISTER(data::MeteorWeaponData);

#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
	/// <summary>
	/// Meteor型武器のマスタデータ（CSV/DB）。
	/// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
	/// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
	/// CSV ヘッダー名は各フィールド名と完全一致すること。
	///
	/// 発動トリガーが無く、FireInterval秒ごとにSearchRadius内の敵からランダムに
	/// 最大MeteorCount体を選び、それぞれの頭上へ隕石を落として範囲ダメージを与える
	/// （MeteorWeaponSystemが担当）。狙い・移動を必要としない完全自動の武器
	/// （パーク選択でのみ取得可能）。
	/// </summary>
	struct MeteorWeaponData
	{
		int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
		std::string Name;                    // 表示・デバッグ用

		float       FireInterval = 4.0f;     // 発動間隔(秒)。CooldownRateで乗算短縮される

		float       Damage = 20.0f;          // このレベルでの火力

		float       Radius = 12.0f;          // このレベルでのエフェクト見た目基準半径(m)

		float       HitRadiusMultiplier = 1.5f; // 実際の当たり判定半径 = Radius×この値

		float       SearchRadius = 60.0f;    // この範囲内の敵を対象候補にする
		int         MeteorCount = 3;         // 1回の発動で落とす隕石の数(=対象数の上限)

		float       HeightOffset = 0.0f;     // 着弾位置のY座標オフセット(m)

		// エフェクト素材ID(';'区切りで複数指定可、data::EffectAssetData参照)。
		// ecs::effectutil::ResolveEffectIds()でパス文字列へ解決してから使うこと。
		std::string EffectIds;               // 着弾エフェクト

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

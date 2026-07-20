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

		/// <summary>
		/// 抽選の重み。大きいほど選択肢に出やすい。0以下でそのパーク種別は出現しなくなる。
		///
		/// 「その他」枠(3〜5番目)の抽選にのみ影響する。1・2番目の枠は
		/// 新武器獲得・武器レベルアップで固定されているため、この値の影響を受けない
		/// (PerkSelectSystem::EnterPerkSelect参照)。
		///
		/// 重みは相対値であり、合計が1になる必要はない
		/// (例: A=2.0, B=1.0 ならAはBの2倍出やすい)。
		/// ImGuiの「Perk Master」から編集し、CSV/DBへ保存できる。
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

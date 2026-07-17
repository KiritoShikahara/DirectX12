#pragma once

#include<vector>

namespace ecs
{
	/// <summary>
	/// プレイヤーがこれまでに選択したパークの、GetPerkPool()内インデックスごとの選択回数。
	/// PerkSelectSystemがパーク種別ごとの最大レベル(data::PerkData::MaxLevel)判定に使う。
	///
	/// プールのインデックス単位で数えるため、AcquireWeaponのように同じePerkEffectTypeを
	/// 複数のプールエントリ(武器種別ごと)が共有していても、エントリごとに独立して数えられる
	/// (他の種別は1種別につきプールエントリが1つのため、インデックス単位=種別単位で一致する)。
	/// GameSceneFactory::CreatePlayerでGetPerkPool().size()分に初期化する。
	/// </summary>
	struct PlayerPerkLevelComponent
	{
		std::vector<int> PickCounts;
	};
}

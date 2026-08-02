#pragma once

#include<vector>

namespace ecs
{
	///<summary>
	///プレイヤーがこれまでに選択したパークの、GetPerkPool内インデックスごとの選択回数。PerkSelectSystemがパーク種別ごとの最大レベル判定に使う
	///</summary>
	struct PlayerPerkLevelComponent
	{
		std::vector<int> PickCounts;
	};
}

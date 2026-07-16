#pragma once

#include<array>

namespace ecs
{
	/// <summary>
	/// GameState::PerkSelect の間だけ存在するランタイム状態。
	/// GameStateComponent と同じ状態管理エンティティに付与する。
	/// PerkSelectSystem が生成(PerkSelectへ入った最初のフレーム)・破棄(選択確定時)を管理する。
	/// </summary>
	struct PerkSelectComponent
	{
		static constexpr int kChoiceCount = 3;

		/// <summary>今回提示している選択肢(GetPerkPool()内のインデックス)</summary>
		std::array<int, kChoiceCount> ChoiceIndices{};

		/// <summary>現在カーソルが当たっている選択肢(0..kChoiceCount-1)</summary>
		int SelectedIndex = 0;
	};

	/// <summary>パーク選択UIのテキストエンティティに付与するタグ</summary>
	struct PerkOptionUiTag
	{
		/// <summary>対応する選択肢インデックス(0..PerkSelectComponent::kChoiceCount-1)</summary>
		int OptionIndex = 0;
	};
}

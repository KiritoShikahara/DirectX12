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
		/// <summary>
		/// 提示する選択肢の数。
		/// 1番目=新武器獲得、2番目=武器レベルアップ、3番目以降=その他、という
		/// 枠の役割分担がある(PerkSelectSystem::EnterPerkSelect参照)。
		/// </summary>
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

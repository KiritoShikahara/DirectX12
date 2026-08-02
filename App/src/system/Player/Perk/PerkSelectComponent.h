#pragma once

#include<array>

namespace ecs
{
	///<summary>
	///GameState::PerkSelectの間だけ存在するランタイム状態。PerkSelectSystemが生成・破棄を管理する
	///</summary>
	struct PerkSelectComponent
	{
		///<summary>
		///デフォルトの選択肢数
		///</summary>
		static constexpr int kDefaultChoiceCount = 3;

		///<summary>
		///ショップ強化パーク選択肢+1取得時の選択肢数。ChoiceIndicesの固定サイズ上限としても使う
		///</summary>
		static constexpr int kMaxChoiceCount = 4;

		///<summary>
		///今回実際に提示している選択肢の数。1番目は新武器獲得か武器レベルアップかの半々ランダム、2番目以降はそれらも含めた全種別からの重み付きランダムという枠の役割分担がある
		///</summary>
		int ChoiceCount = kDefaultChoiceCount;

		///<summary>
		///今回提示している選択肢、GetPerkPool内のインデックス。有効な範囲は0からChoiceCount-1
		///</summary>
		std::array<int, kMaxChoiceCount> ChoiceIndices{};

		///<summary>
		///現在カーソルが当たっている選択肢、0からChoiceCount-1
		///</summary>
		int SelectedIndex = 0;
	};

	///<summary>
	///パーク選択UIのテキストエンティティに付与するタグ
	///</summary>
	struct PerkOptionUiTag
	{
		///<summary>
		///対応する選択肢インデックス、0からPerkSelectComponent::ChoiceCount-1
		///</summary>
		int OptionIndex = 0;
	};
}

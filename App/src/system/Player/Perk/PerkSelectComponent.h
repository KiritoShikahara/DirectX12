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
		/// <summary>デフォルトの選択肢数</summary>
		static constexpr int kDefaultChoiceCount = 3;

		/// <summary>ショップ強化「パーク選択肢+1」(data::eStatUpgradeType::PerkChoiceCount)取得時の選択肢数。
		/// ChoiceIndicesの固定サイズ上限としても使う</summary>
		static constexpr int kMaxChoiceCount = 4;

		/// <summary>
		/// 今回実際に提示している選択肢の数(kDefaultChoiceCountまたはkMaxChoiceCount。
		/// PerkSelectSystem::EnterPerkSelectがPlayerSaveData::PerkChoiceCountLevelを見て決める)。
		/// 1番目=新武器獲得か武器レベルアップかを半々のランダム、2番目以降=それらも含めた
		/// 全種別からの重み付きランダム、という枠の役割分担がある(PerkSelectSystem::EnterPerkSelect参照)。
		/// </summary>
		int ChoiceCount = kDefaultChoiceCount;

		/// <summary>今回提示している選択肢(GetPerkPool()内のインデックス)。有効な範囲は0..ChoiceCount-1
		/// (ChoiceCount<kMaxChoiceCountの場合、末尾の未使用分は初期値のままになる)</summary>
		std::array<int, kMaxChoiceCount> ChoiceIndices{};

		/// <summary>現在カーソルが当たっている選択肢(0..ChoiceCount-1)</summary>
		int SelectedIndex = 0;
	};

	/// <summary>パーク選択UIのテキストエンティティに付与するタグ</summary>
	struct PerkOptionUiTag
	{
		/// <summary>対応する選択肢インデックス(0..PerkSelectComponent::ChoiceCount-1)</summary>
		int OptionIndex = 0;
	};
}

#pragma once

namespace ecs
{
	/// <summary>
	/// ハブ画面(HubScene)のカーソル位置を保持するコンポーネント。
	/// 選択肢は固定2件（0:武器・ステージ選択、1:ステータス強化）。
	/// </summary>
	struct HubMenuComponent
	{
		static constexpr int kChoiceCount = 2;

		/// <summary>現在カーソルが当たっている選択肢(0..kChoiceCount-1)</summary>
		int SelectedIndex = 0;
	};

	/// <summary>ハブ画面の選択肢テキストエンティティに付与するタグ</summary>
	struct HubMenuOptionUiTag
	{
		/// <summary>対応する選択肢インデックス(0..HubMenuComponent::kChoiceCount-1)</summary>
		int OptionIndex = 0;
	};

	/// <summary>ハブ画面の操作案内テキストに付与するタグ。「Selectって何ボタン？」とならないよう、
	/// HubMenuInputSystemが最後に使われた入力デバイスに応じて毎フレーム内容を更新する</summary>
	struct HubGuideUiTag
	{
	};
}

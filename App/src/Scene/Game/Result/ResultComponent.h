#pragma once

namespace ecs
{
	/// <summary>
	/// GameState::Result の間だけ存在するランタイム状態。
	/// GameStateComponent と同じ状態管理エンティティに付与する。
	/// ResultSystem が生成(Resultへ入った最初のフレーム)・確定時のシーン遷移を管理する。
	/// </summary>
	struct ResultComponent
	{
		/// <summary>ゲームオーバー時の2択(Retry/Title)でのみ使用。0=Retry, 1=Title</summary>
		int SelectedIndex = 0;
	};

	/// <summary>
	/// ゲームオーバー時の選択肢UI(Retry/Title)のテキストエンティティに付与するタグ。
	/// </summary>
	struct ResultOptionUiTag
	{
		/// <summary>0=Retry, 1=Title</summary>
		int OptionIndex = 0;
	};
}

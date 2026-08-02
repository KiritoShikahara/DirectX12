#pragma once

namespace ecs
{
	///<summary>
	///GameState::Resultの間だけ存在するランタイム状態。ResultSystemが生成し確定時のシーン遷移を管理する
	///</summary>
	struct ResultComponent
	{
		///<summary>
		///ゲームオーバー時の2択、Retry/Titleでのみ使用。0=Retry、1=Title
		///</summary>
		int SelectedIndex = 0;
	};

	///<summary>
	///ゲームオーバー時の選択肢UI、Retry/Titleのテキストエンティティに付与するタグ
	///</summary>
	struct ResultOptionUiTag
	{
		///<summary>
		///0=Retry、1=Title
		///</summary>
		int OptionIndex = 0;
	};
}

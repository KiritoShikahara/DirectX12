#pragma once

#include<cstdint>

namespace ecs
{
	///<summary>
	///メニューの状態変数保持用
	///</summary>
	struct MenuControllerComp
	{
		uint32_t CurrentlySelectedIdx = 0; // 現在表示中のページインデックス
		uint32_t TotalPages = 0;	// ロードされたページ数、CSVのデータサイズから取得
		float WindowWidth = 1920.0; // スライドさせる幅、Windowの仮想サイズ

		uint32_t ActiveSpellID = 0; // 今表示されているスペルのID
	};

	///<summary>
	///自身のスペルのID用。画像と一緒にアタッチする
	///</summary>
	struct SpellMenuDataComp
	{
		uint32_t SpellID = 0;
		uint32_t PageIndex = 0;
	};

	///<summary>
	///画面外から画面内に移動する用
	///</summary>
	struct MenuSlideComp
	{
		float TargetX = 0.0f;  // このページが目指すべき目標X座標
		float SlideSpeed = 12.0f; // 補間スピード
	};

	///<summary>
	///初期武器選択画面の操作案内テキストに付与するタグ。MenuInputSystemが最後に使われた入力デバイスに応じて毎フレーム内容を更新する
	///</summary>
	struct MenuGuideUiTag
	{
	};
}
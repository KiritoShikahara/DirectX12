#pragma once

namespace ecs
{
	/// <summary>
	/// ゲームの状態
	/// </summary>
	enum class eGameState
	{
		PreStart, // 開始前
		InGame, // ゲーム中
		PerkSelect, // パーク選択
		Result, // リザルト
	};

	/// <summary>
	/// ゲームの状態コンポーネント
	/// </summary>
	struct GameStateComponent
	{
		eGameState GameState = eGameState::PreStart; // 今の状態

		bool PerkSelectRequested = false; // パーク選択への遷移リクエスト
		bool InGameRequested = false; // ゲーム状態への遷移リクエスト
	};
}
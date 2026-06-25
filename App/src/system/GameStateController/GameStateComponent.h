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
		eGameState GameState = eGameState::PreStart;
	};
}
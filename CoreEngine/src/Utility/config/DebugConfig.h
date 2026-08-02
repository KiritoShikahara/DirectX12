#pragma once


// 開発用ツール有効時用のマクロ
// 現状はDebugとDevelopで動作
#if defined(_DEBUG) || (defined(ECSE_DEV_TOOL) && ECSE_DEV_TOOL)
#define DEV_TOOL_ENABLED  (1)
#else
#define DEV_TOOL_ENABLED  (0)
#endif

// アサートは最適化の有無で挙動が変わると困るため、Debugビルドでのみ有効にする
#if defined(_DEBUG)
#define ENABLE_ASSERT     (1)
#else
#define ENABLE_ASSERT     (0)
#endif

#define DEBUG_DRAW_COLLISION (DEV_TOOL_ENABLED)


// デバッグビルドでのみ有効な設定やマクロをここに定義する
#ifdef _DEBUG

/*
* 入力
*/
#define ENABLE_INPUT_DEBUG_SHOW
	#ifdef ENABLE_INPUT_DEBUG_SHOW
	#define ENABLE_INPUT_DEBUG_KEYBOARD  // キーボードの表示
	#define ENABLE_INPUT_DEBUG_MOUSE     // マウスの表示
	#define ENABLE_INPUT_DEBUG_PAD       // パッドの表示
	#endif

// Release
#else

#endif // _DEBUG

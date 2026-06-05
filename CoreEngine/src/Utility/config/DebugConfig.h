#pragma once


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

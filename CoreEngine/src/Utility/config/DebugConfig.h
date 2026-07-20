#pragma once


// ============================================================================
//  開発ツール(ImGuiデバッグUI・System別の所要時間計測など)の有効/無効。
//
//  【重要】このマクロを参照する箇所は必ずこのヘッダを直接includeすること。
//  クラスのメンバ変数をこのマクロで条件付きにしている箇所があるため
//  (ComponentSystemManager::mSystemTimings 等)、翻訳単位ごとにマクロの見え方が
//  食い違うとクラスのサイズが変わり、ODR違反によるメモリ破壊を引き起こす。
//  実際に「CoreEngine側だけがpch.hでこのマクロを定義しており、App側は未定義」
//  という状態でReleaseビルドが起動直後にアクセス違反で落ちる事故が起きた
//  (リンカのC4743警告「異なるサイズを含んでいます: 128 および 64 バイト」が兆候)。
//
//  【重要】判定は必ず #if DEV_TOOL_ENABLED と値で行うこと。
//  Releaseでも (0) として「定義はされる」ため、#if defined(DEV_TOOL_ENABLED) と
//  書くと常に真になってしまい、意図と逆の結果になる。
// ============================================================================
//  ビルド構成と開発ツールの対応:
//    Debug   … _DEBUG あり            → 開発ツール有効 (最適化なし。ライブラリ挙動もDebug版)
//    Develop … ECSE_DEV_TOOL=1 を定義  → 開発ツール有効 (最適化はReleaseと同一)
//    Release … どちらも無し            → 開発ツール無効 (製品版)
//
//  Develop構成は「Releaseと同じ最適化・同じライブラリ挙動のままImGuiで計測したい」
//  ために存在する。Debugビルドの数値は最適化無効+イテレータデバッグの影響で
//  実性能と数十倍乖離するため、性能判断はDevelopまたはReleaseで行うこと。
//
//  判定は必ず #if DEV_TOOL_ENABLED と書くこと。_DEBUG はこのマクロに畳み込み済みなので
//  #if defined(_DEBUG) || ... のように書き足す必要はない(書き分けると条件が食い違い、
//  「初期化はされないのに描画だけ走る」といった不整合の温床になる)。
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

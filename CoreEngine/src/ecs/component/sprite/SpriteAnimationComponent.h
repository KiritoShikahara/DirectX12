#pragma once

#include<Utility/Export/Export.h>

namespace ecs
{
	/// <summary>
	/// スプライトシートのコマ送りアニメーションを行うためのコンポーネント。
	/// Sprite::UVScale / Sprite::UVOffset を毎フレーム書き換える対象として使う。
	/// 適用には対応する SpriteAnimationSystem を Update ループで呼ぶ必要がある。
	/// </summary>
	struct ENGINE_API SpriteAnimation
	{
		/// <summary>シートの列数（横方向のコマ数）</summary>
		int Columns = 1;

		/// <summary>シートの行数（縦方向のコマ数）</summary>
		int Rows = 1;

		/// <summary>使用する先頭フレーム（0始まり、Columns*Rows のうちの何コマ目から）</summary>
		int StartFrame = 0;

		/// <summary>再生する総コマ数。0以下なら Columns*Rows 全体を使用する。</summary>
		int FrameCount = 0;

		/// <summary>1コマあたりの表示時間（秒）</summary>
		float FrameDuration = 0.1f;

		/// <summary>true ならループ再生。false なら最終コマで停止する。</summary>
		bool IsLooping = true;

		/// <summary>再生中フラグ。false の間はシステムが更新をスキップする。</summary>
		bool IsPlaying = true;

		/// <summary>現在のコマ内経過時間（秒）。システムが内部で更新する。</summary>
		float ElapsedTime = 0.0f;

		/// <summary>現在のコマインデックス（StartFrame からの相対値、0始まり）。システムが内部で更新する。</summary>
		int CurrentFrame = 0;

		/// <summary>
		/// アニメーション状態を先頭フレームにリセットする。
		/// </summary>
		void Reset()
		{
			ElapsedTime = 0.0f;
			CurrentFrame = 0;
		}
	};
}
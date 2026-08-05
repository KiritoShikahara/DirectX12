#pragma once

namespace ecs
{
	///<summary>
	///InGame開始時に画面中央へ出す「Start」演出テキストのフェード状態を保持する。
	///StartEffectSystemが経過時間からアルファ値を計算し、フェードアウト完了後にエンティティを破棄する
	///</summary>
	struct StartEffectComponent
	{
		///<summary>経過時間、秒</summary>
		float Elapsed = 0.0f;

		///<summary>フェードインにかける時間、秒</summary>
		float FadeInDuration = 0.4f;

		///<summary>最大不透明度を維持する時間、秒</summary>
		float HoldDuration = 0.8f;

		///<summary>フェードアウトにかける時間、秒</summary>
		float FadeOutDuration = 0.6f;
	};
}

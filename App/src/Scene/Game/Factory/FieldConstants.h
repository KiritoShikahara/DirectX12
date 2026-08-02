#pragma once

namespace ecs
{
	///<summary>
	///フィールドFbx関連の共有定数。CreateGroundとCreateFieldBoundaryの両方から参照し値のズレを防ぐ
	///</summary>
	namespace FieldConstants
	{
		///<summary>
		///Field.fbxの生データの半径、スケール適用前。頂点座標範囲の実測値
		///</summary>
		constexpr float kRawHalfExtent = 50.0f;

		///<summary>
		///CreateGroundで適用するスケール。ワールド半径はkRawHalfExtent×kScaleで決まる
		///</summary>
		constexpr float kScale = 40.0f;

		///<summary>
		///ワールド空間でのフィールドの半径。見た目の地面メッシュがここまで存在する
		///</summary>
		constexpr float kWorldHalfExtent = kRawHalfExtent * kScale;

		///<summary>
		///プレイヤーが実際に移動できる境界の半径。地面の縁が見えないよう見た目のフィールド半径より内側に壁を置く
		///</summary>
		constexpr float kPlayableHalfExtent = kWorldHalfExtent - 100.0f;

		///<summary>
		///見えない境界壁の厚み、半分の値。壁本体の見た目は使わないため大きさは重要でない
		///</summary>
		constexpr float kWallHalfThickness = 50.0f;

		///<summary>
		///見えない境界壁の高さ、半分の値。必殺技の上昇演出より高く設定し発動時に衝突しないよう余裕を持たせる
		///</summary>
		constexpr float kWallHalfHeight = 600.0f;
	}
}

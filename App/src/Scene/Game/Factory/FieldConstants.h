#pragma once

namespace ecs
{
	/// <summary>
	/// フィールド(Assets/Fbx/Field/Field.fbx)関連の共有定数。
	/// GameSceneFactory::CreateGround(地面の生成)とCreateFieldBoundary(見えない境界壁の生成)の
	/// 両方から参照するため、値のズレ(地面と壁の位置が合わない等)を防ぐためにここへ集約する。
	/// </summary>
	namespace FieldConstants
	{
		/// <summary>
		/// Field.fbxの生データの半径(X/Z、スケール適用前)。
		/// Field.fbx.bin の頂点座標範囲(X/Z共に[-50,50])を実測して求めた値。
		/// </summary>
		constexpr float kRawHalfExtent = 50.0f;

		/// <summary>
		/// CreateGroundで適用するスケール。
		/// 【2026-07-24】フィールドを拡張する要望により、旧値20から2倍(40)に変更。
		/// ワールド半径は kRawHalfExtent(50) × 40 = 2000 → 一辺4000ユニットになる。
		/// </summary>
		constexpr float kScale = 40.0f;

		/// <summary>ワールド空間でのフィールドの半径(X/Z)。見た目の地面メッシュがここまで存在する</summary>
		constexpr float kWorldHalfExtent = kRawHalfExtent * kScale;

		/// <summary>
		/// プレイヤーが実際に移動できる境界の半径(ワールド)。
		/// フィールドの縁ギリギリまで許容すると、境界壁と地面の終端の間に隙間ができたり
		/// 地面の縁が見えてしまうため、見た目のフィールド半径より少し内側に見えない壁を置く。
		/// </summary>
		constexpr float kPlayableHalfExtent = kWorldHalfExtent - 100.0f;

		/// <summary>見えない境界壁の厚み(半分の値)。壁本体の見た目は使わないため大きさは重要でない</summary>
		constexpr float kWallHalfThickness = 50.0f;

		/// <summary>
		/// 見えない境界壁の高さ(半分の値)。
		/// 必殺技の上昇演出(UltimateData::RiseHeight=450)より高く設定し、
		/// 発動時にすり抜けたり衝突したりしないよう十分な余裕を持たせる。
		/// </summary>
		constexpr float kWallHalfHeight = 600.0f;
	}
}

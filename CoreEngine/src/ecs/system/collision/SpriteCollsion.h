#pragma once

#include<DirectXMath.h>

namespace ecs
{
	struct Transform;
	struct Sprite;
}

namespace ecs
{
	/// <summary>
	/// 2D スプライトに関する当たり判定（カーソル・矩形・点など）をまとめるクラス。
	/// 全メソッドは static で、状態を持たない。
	/// </summary>
	class SpriteCollision
	{
	public:
		SpriteCollision() = delete;

		/// <summary>
		/// 指定座標（スクリーン/ワールド座標、Sprite と同じ空間）がスプライトの矩形範囲内にあるかを判定する。
		/// Transform の回転・Sprite の Pivot / DrawScale / Flip を考慮した OBB（回転矩形）判定。
		/// SpriteRenderer::CalculateShaderData と同じワールド変換規則
		/// （Pivot → Scale+Flip → RotationZ → Translation）を前提とする。
		/// </summary>
		/// <param name="tr">対象の Transform</param>
		/// <param name="sp">対象の Sprite</param>
		/// <param name="point">判定したい座標（マウス座標など）</param>
		/// <returns>true: 矩形範囲内</returns>
		static bool IsPointOver(const ecs::Transform& tr, const ecs::Sprite& sp, DirectX::XMFLOAT2 point);

		/// <summary>
		/// マウス座標がスプライトの矩形範囲内にあるかを判定する。
		/// IsPointOver の別名（呼び出し意図を明確にするためのラッパー）。
		/// </summary>
		/// <param name="tr">対象の Transform</param>
		/// <param name="sp">対象の Sprite</param>
		/// <param name="mousePos">マウス座標</param>
		/// <returns>true: マウスがスプライト上にある</returns>
		static bool IsMouseOver(const ecs::Transform& tr, const ecs::Sprite& sp);

	private:
		/// <summary>
		/// 点をスプライトのローカル空間（Pivot 適用前の単位クワッド空間 0,0?1,1）に変換する。
		/// ワールド行列の逆行列を用いて回転・スケール・平行移動を全て打ち消す。
		/// </summary>
		/// <param name="tr">対象の Transform</param>
		/// <param name="sp">対象の Sprite</param>
		/// <param name="point">変換したいワールド座標</param>
		/// <param name="outLocal">変換後のローカル座標（成功時のみ書き込まれる）</param>
		/// <returns>true: 変換に成功（逆行列が存在した）</returns>
		static bool TryTransformToLocal(
			const ecs::Transform& tr,
			const ecs::Sprite& sp,
			DirectX::XMFLOAT2 point,
			DirectX::XMFLOAT2& outLocal);
	};
}
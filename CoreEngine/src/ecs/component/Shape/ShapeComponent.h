#pragma once
#include"../sprite/SpriteComponent.h"

namespace ecs
{
	/// <summary>
	/// 図形描画の種類。PS_Shape.hlsl 側の分岐と値を一致させること。
	/// </summary>
	enum class ENGINE_API ShapeType : int
	{
		Rect = 0,
		Circle = 1,
		Triangle = 2,
	};

	/// <summary>
	/// テクスチャを使わない単色図形描画コンポーネント。
	/// </summary>
	struct ENGINE_API Shape
	{
		/// <summary>描画色</summary>
		graphics::Color Color = graphics::Color::White;

		/// <summary>
		/// 基準点（ピボット）。
		/// {0,0} = 左上, {0.5,0.5} = 中央, {1,1} = 右下
		/// </summary>
		DirectX::XMFLOAT2     Pivot = { 0.5f, 0.5f };

		/// <summary>
		/// 描画サイズ（ピクセル）。
		/// </summary>
		DirectX::XMFLOAT2     Size = { 100.0f, 100.0f };

		/// <summary>Size に対する追加倍率</summary>
		DirectX::XMFLOAT2     DrawScale = { 1.0f, 1.0f };

		/// <summary>
		/// 反転フラグ。
		/// X=-1 で水平反転, Y=-1 で垂直反転。
		/// </summary>
		DirectX::XMFLOAT2     Flip = { 1.0f, 1.0f };

		/// <summary>輝度（単位倍率）</summary>
		float                 Intensity = 1.0f;

		/// <summary>
		/// 表示割合（0.0～1.0）。FType によって削れ方が決まる。
		/// 1.0=全体表示、0.0=非表示と同等。
		/// </summary>
		float                 FillAmount = 1.0f;

		/// <summary>
		/// FillAmount の削れ方の種類。
		/// Horizontal=水平方向（左→右）、Radial=時計回りの円形ゲージ。
		/// ecs::FillType（SpriteComponent.h で定義）を共有利用。
		/// </summary>
		FillType              FType = FillType::Horizontal;

		/// <summary>
		/// 描画順。値が小さいほど手前。
		/// SetLayer() で RenderLayer ベースで設定することを推奨。
		/// </summary>
		int Layer = static_cast<int>(SpriteLayer::Character);

		/// <summary>図形の種類</summary>
		ShapeType Type = ShapeType::Rect;

		/// <summary>表示フラグ</summary>
		bool                  IsVisible = true;

		Shape() = default;
		explicit Shape(ShapeType type) : Type(type) {}

		/// <summary>
		/// レイヤー設定用のヘルパー
		/// </summary>
		void SetLayer(SpriteLayer base, int offset = 0)
		{
			Layer = static_cast<int>(base) + offset;
		}
	};
}
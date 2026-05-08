#pragma once

#include<Utility/Export/Export.h>
#include<graphics/Color/Color.h>
#include<DirectXMath.h>

namespace graphics
{
	class Texture;
}

namespace ecs
{
	enum class ENGINE_API SpriteLayer : int
	{
		Background = 0,
		Map = 1000,
		Character = 2000,
		Effect = 3000,
		UI = 4000,
		System = 5000,
	};

	/// <summary>
	/// スプライト描画に必要なパラメータをまとめたコンポーネント。
	/// </summary>
	struct ENGINE_API Sprite
	{
		/// <summary>乗算カラー</summary>
		graphics::Color Color = graphics::Color::White;

		/// <summary>
		/// 基準点（ピボット）。
		/// {0,0} = 左上, {0.5,0.5} = 中央, {1,1} = 右下
		/// </summary>
		DirectX::XMFLOAT2     Pivot = { 0.0f, 0.0f };

		/// <summary>
		/// 描画サイズ（ピクセル）。
		/// {0,0} のときはテクスチャの実サイズを使用する。
		/// </summary>
		DirectX::XMFLOAT2     Size = { 0.0f, 0.0f };

		/// <summary>Size に対する追加倍率</summary>
		DirectX::XMFLOAT2     DrawScale = { 1.0f, 1.0f };

		/// <summary>
		/// 反転フラグ。
		/// X=-1 で水平反転, Y=-1 で垂直反転。
		/// </summary>
		DirectX::XMFLOAT2     Flip = { 1.0f, 1.0f };

		/// <summary>光度（輝度倍率）</summary>
		float                 Intensity = 1.0f;

		/// <summary>
		/// 描画順。値が小さいほど手前。
		/// SetLayer() で RenderLayer ベースで設定することを推奨。
		/// </summary>
		int Layer = static_cast<int>(SpriteLayer::Character);

		/// <summary>テクスチャリソース（非所有）</summary>
		graphics::Texture* Texture = nullptr;

		/// <summary>表示フラグ</summary>
		bool                  IsVisible = true;

		/// <summary>
		/// コンストラクタ。テクスチャの実サイズを取得したいのと依存注入
		/// </summary>
		explicit Sprite(graphics::Texture* texture);

		/// <summary>
		/// レイヤー設定用のヘルパー
		/// </summary>
		/// <param name="base"></param>
		/// <param name="offset"></param>
		void SetLayer(SpriteLayer base, int offset = 0);


	};
}
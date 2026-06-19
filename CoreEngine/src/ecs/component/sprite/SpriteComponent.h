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

	enum class ENGINE_API FillType : int
	{
		Horizontal = 0, // 左から右
		Radial = 1,	// 時計回り
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

		/// <summary>
		/// テクスチャUV切り出し範囲のスケール（テクスチャ全体に対する比率）。
		/// {1,1} で全体表示。スプライトシートの1コマの大きさに相当。
		/// 例: 横4列・縦2行のシートなら {0.25f, 0.5f}。
		/// </summary>
		DirectX::XMFLOAT2     UVScale = { 1.0f, 1.0f };

		/// <summary>
		/// テクスチャUV切り出し範囲のオフセット（テクスチャ全体に対する比率、左上原点）。
		/// UVScale と組み合わせてスプライトシート上の特定コマを指す。
		/// 通常は SpriteAnimationComponent / アニメ用システムが書き換える。
		/// </summary>
		DirectX::XMFLOAT2     UVOffset = { 0.0f, 0.0f };

		/// <summary>光度（輝度倍率）</summary>
		float                 Intensity = 1.0f;

		/// <summary>
		/// 0.0:非表示 0.5:半分 1.0:全体
		/// </summary>
		float                 FillAmount = 1.0f;

		/// <summary>
		/// Horizontal:右から削れてく 
		/// Radial: 時計回りに削れてく
		/// </summary>
		FillType              FType = FillType::Horizontal;

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

		/// <summary>
		/// スプライトシートの行列数を指定して、UVScale を一括設定するヘルパー。
		/// 例: 横4列・縦2行のシートなら SetSheetGrid(4, 2) を1回呼ぶ。
		/// </summary>
		/// <param name="columns">シートの列数</param>
		/// <param name="rows">シートの行数</param>
		void SetSheetGrid(int columns, int rows);

		/// <summary>
		/// 0始まりのフレーム番号から UVOffset を計算して設定するヘルパー。
		/// SetSheetGrid() で columns を設定済みであることが前提。
		/// </summary>
		/// <param name="frameIndex">0始まりのコマ番号（左上から右へ、行ごとに進む）</param>
		/// <param name="columns">シートの列数（UVScale.x の逆数）</param>
		void SetFrame(int frameIndex, int columns);
	};
}
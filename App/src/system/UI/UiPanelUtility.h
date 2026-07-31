#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<string>
#include<vector>

namespace ecs::uiutil
{
	/// <summary>
	/// 背景(タイトル画像等)の上に文字/アイコンを直接乗せると読みづらくなるのを防ぐための、
	/// 黒半透明の板を敷くユーティリティ。白テクスチャ(Assets/Effect/Texture/White.png)を
	/// Sprite::Colorで黒に着色して代用する手法(元はPerkSelectSystem/HubScene/StatusUpgradeSceneに
	/// それぞれ個別実装されていたものを共通化)。
	///
	/// 描画順は「Layerが小さいほど奥」(SpriteRenderer参照)。上に乗せる文字/アイコンの
	/// layerOffsetより必ず小さい値を渡すこと。
	/// </summary>
	/// <param name="centerX">パネル中心のX座標(仮想解像度基準)</param>
	/// <param name="centerY">パネル中心のY座標(仮想解像度基準)</param>
	/// <param name="width">パネル幅(px)</param>
	/// <param name="height">パネル高さ(px)</param>
	/// <param name="layerOffset">SpriteLayer::UIからのオフセット</param>
	/// <param name="alpha">不透明度(0=透明,1=不透明)。既定0.6は読みやすさ用ウィンドウの標準値
	/// (確認ダイアログのように背景を完全に隠したい場合は1.0を渡す)</param>
	/// <returns>生成したパネルエンティティ(呼び出し側でタグ付け・表示切替する場合に使う)</returns>
	entt::entity CreateTranslucentPanel(
		float centerX, float centerY,
		float width, float height,
		int layerOffset,
		float alpha = 0.6f);

	/// <summary>テキスト行の水平揃え方向</summary>
	enum class eTextHorizontalAlign
	{
		Left,   // xを各行の左端として使う
		Center, // xを各行の中心として使い、実測幅(TextRenderer::MeasureWidth)から左端を逆算する
	};

	/// <summary>
	/// 複数行のテキストエンティティをまとめて生成する汎用ヘルパー。元は
	/// OptionsMenuSystem::BuildControlsInfoLines(左揃え・複数行の操作方法一覧)と
	/// PerkSelectSystemの操作説明(中央揃え・1行)にそれぞれ個別実装されていたものを
	/// 共通化した(CreateTranslucentPanelと同じ経緯)。全行同じスタイル(サイズ・色・レイヤー)で、
	/// startYから1行ごとにlineSpacingずつ下へ並べる(1行のみの場合lineSpacingは使われない)。
	/// </summary>
	/// <param name="lines">表示する行(上から順)</param>
	/// <param name="align">水平揃え方向</param>
	/// <param name="x">Left: 各行の左端X座標 / Center: 各行の中心X座標</param>
	/// <param name="startY">1行目のY座標</param>
	/// <param name="lineSpacing">行間(px)</param>
	/// <param name="size">文字サイズ</param>
	/// <param name="color">文字色</param>
	/// <param name="layer">TextComponent::Layer</param>
	/// <returns>生成した各行のエンティティ(linesと同じ順序。呼び出し側でタグ付け・
	/// 破棄管理する場合に使う)</returns>
	std::vector<entt::entity> CreateTextLines(
		const std::vector<std::wstring>& lines,
		eTextHorizontalAlign align,
		float x, float startY, float lineSpacing,
		float size, const DirectX::XMFLOAT4& color, int layer);
}

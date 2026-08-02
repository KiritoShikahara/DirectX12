#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<string>
#include<vector>

namespace ecs::uiutil
{
	///<summary>
	///背景画像の上に表示する文字やアイコンを見やすくするための半透明パネルを生成
	///</summary>
	///<param name="centerX">パネル中心のX座標</param>
	///<param name="centerY">パネル中心のY座標</param>
	///<param name="width">パネルの幅</param>
	///<param name="height">パネルの高さ</param>
	///<param name="layerOffset">UIレイヤーからのオフセット</param>
	///<param name="alpha">パネルの不透明度</param>
	///<returns>生成したパネルエンティティ</returns>
	entt::entity CreateTranslucentPanel(
		float centerX, float centerY,
		float width, float height,
		int layerOffset,
		float alpha = 0.6f);

	///<summary>
	///テキストの水平方向の配置
	///</summary>
	enum class eTextHorizontalAlign
	{
		Left,
		Center,
	};

	///<summary>
	///複数行のテキストエンティティをまとめて生成
	///</summary>
	///<param name="lines">表示する文字列一覧</param>
	///<param name="align">テキストの水平方向の配置</param>
	///<param name="x">基準となるX座標</param>
	///<param name="startY">先頭行のY座標</param>
	///<param name="lineSpacing">行間</param>
	///<param name="size">文字サイズ</param>
	///<param name="color">文字色</param>
	///<param name="layer">描画レイヤー</param>
	///<returns>生成したテキストエンティティ一覧</returns>
	std::vector<entt::entity> CreateTextLines(
		const std::vector<std::wstring>& lines,
		eTextHorizontalAlign align,
		float x, float startY, float lineSpacing,
		float size, const DirectX::XMFLOAT4& color, int layer);
}
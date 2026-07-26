#pragma once

#include<entt/entt.hpp>

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
}

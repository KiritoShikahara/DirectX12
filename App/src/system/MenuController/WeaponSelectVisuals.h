#pragma once

#include<graphics/Color/Color.h>

namespace ecs::menuvisuals
{
	/// <summary>
	/// 初期武器選択画面(MenuScene)の武器イメージカラー。SpellMenuData::ID
	/// (Assets/Bin/CSV/MenuSpells.csv、GameSceneFactory::CreatePlayerのID対応表と同じ値)ごとに
	/// Fireは赤、Lightning(内部データはAreaAttack/IceSpike)は青、Orbは黄を返す。
	/// MenuScene::CreateSpells()(カード背景円の色)とMenuPagingSystem(画面全体の色付き
	/// オーバーレイ)の両方が参照するため、どちらか一方の.cppにローカルで持たず共有ヘッダーへ切り出す。
	/// </summary>
	inline graphics::Color GetWeaponAccentColor(uint32_t spellId)
	{
		switch (spellId)
		{
		case 1001: return graphics::Color(1.0f, 0.25f, 0.15f, 1.0f); // Fire: 赤
		case 1002: return graphics::Color(0.2f, 0.5f, 1.0f, 1.0f);   // Lightning(Ice): 青
		case 1003: return graphics::Color(1.0f, 0.85f, 0.2f, 1.0f);  // Orb: 黄
		default:   return graphics::Color(1.0f, 1.0f, 1.0f, 1.0f);  // 未知のIDは無着色(白)
		}
	}
}

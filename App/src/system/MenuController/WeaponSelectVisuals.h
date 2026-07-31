#pragma once

#include<graphics/Color/Color.h>

namespace ecs::menuvisuals
{
	/// <summary>
	/// 初期武器選択画面(MenuScene)の武器イメージカラー。SpellMenuData::ID
	/// (Assets/Bin/CSV/MenuSpells.csv、GameSceneFactory::CreatePlayerのID対応表と同じ値)ごとに
	/// Fireは赤、Thunder(内部データはAreaAttack/IceSpike)は青、Orbは黄を返す。
	/// MenuScene::CreateSpells()(カード背景の色)とMenuPagingSystem(画面全体の色付き
	/// オーバーレイ)の両方が参照するため、どちらか一方の.cppにローカルで持たず共有ヘッダーへ切り出す。
	/// </summary>
	inline graphics::Color GetWeaponAccentColor(uint32_t spellId)
	{
		switch (spellId)
		{
		case 1001: return graphics::Color(1.0f, 0.25f, 0.15f, 1.0f); // Fire: 赤
		case 1002: return graphics::Color(0.2f, 0.5f, 1.0f, 1.0f);   // Thunder(Ice): 青
		// Orb: 黄。以前の(1.0,0.95,0.1)はR,G成分がほぼ同値で白に近い比率になり、
		// 輝度もFire/Thunderよりずっと高かったため白っぽく見えていた。
		// Gを大きく下げ、B=0にして明確な黄金色にする(FireやThunderと同程度の輝度感に近づける)。
		case 1003: return graphics::Color(1.0f, 0.75f, 0.0f, 1.0f);
		default:   return graphics::Color(1.0f, 1.0f, 1.0f, 1.0f);  // 未知のIDは無着色(白)
		}
	}
}

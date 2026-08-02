#pragma once

#include<graphics/Color/Color.h>

namespace ecs::menuvisuals
{
	///<summary>
	///初期武器選択画面の武器イメージカラー。SpellMenuData::IDごとにFireは赤、Thunderは青、Orbは黄を返す。MenuSceneとMenuPagingSystemの両方が参照するため共有ヘッダーへ切り出す
	///</summary>
	inline graphics::Color GetWeaponAccentColor(uint32_t spellId)
	{
		switch (spellId)
		{
		case 1001: return graphics::Color(1.0f, 0.25f, 0.15f, 1.0f); // Fire: 赤
		case 1002: return graphics::Color(0.2f, 0.5f, 1.0f, 1.0f);   // Thunder、Iceは青
		// Orbは黄。以前の配色は白っぽく見えていたためGを下げてB=0にし明確な黄金色にした
		case 1003: return graphics::Color(1.0f, 0.75f, 0.0f, 1.0f);
		default:   return graphics::Color(1.0f, 1.0f, 1.0f, 1.0f);  // 未知のIDは無着色、白
		}
	}
}

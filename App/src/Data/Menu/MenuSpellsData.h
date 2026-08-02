#pragma once

#include <string>
#include <Data/Storage/Reflection.h>

namespace data
{
	/// <summary>初期武器選択画面(MenuScene)の1件分。IDと武器アイコン画像のパスを持つ</summary>
	struct SpellMenuData
	{
		int ID = 0;
		std::string TexPath; // 武器アイコン画像のパス

		REFLECT_BEGIN(SpellMenuData, "spell_menu")
			REFLECT_FIELD_ID(ID)
			REFLECT_FIELD_STR(TexPath)
			REFLECT_END()
	};
}

REFLECT_REGISTER(::data::SpellMenuData);
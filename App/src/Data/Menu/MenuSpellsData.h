#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
	/// <summary>
	/// メニュー画面のスペル紹介の画像とID
	/// </summary>
	struct SpellMenuData
	{
		int ID = 0;
		std::string TexPath;

		REFLECT_BEGIN(SpellMenuData, "spell_menu")
			REFLECT_FIELD_ID(ID)
			REFLECT_FIELD_STR(TexPath)
			REFLECT_END()
	};
}

REFLECT_REGISTER(::data::SpellMenuData)
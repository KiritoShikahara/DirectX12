#pragma once

#include<Utility/Export/Export.h>
#include<string>

namespace ecs
{
	/// <summary>
	/// Hierarchyパネル等での表示名。
	/// エディタで配置したエンティティ(PlaceableTag)に付与する。
	/// </summary>
	struct ENGINE_API NameComponent
	{
		std::string Name;
	};
}

#pragma once

#include<Utility/Export/Export.h>
#include<string>

namespace ecs
{
	/// <summary>
	/// FbxComponent::Resource の解決に使ったキー(アセットの .fbx.bin パス、
	/// または PrimitiveResourceManager のプリミティブ名)を保持する。
	/// Resource は生ポインタでシリアライズできないため、
	/// 保存/復元時はこのキーを介して再解決する。
	/// </summary>
	struct ENGINE_API AssetKeyComponent
	{
		std::string Key;
	};
}

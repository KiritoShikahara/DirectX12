#include"pch.h"
#include"DataRegistry.h"

namespace data
{
	void DataRegistry::LoadAll()
	{
		for (const auto& key : mOrder)
		{
			mEntries.at(key).LoadFn();
		}
	}
}
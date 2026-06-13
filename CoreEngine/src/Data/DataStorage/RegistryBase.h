#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <cassert>
#include <string>

namespace data
{

	/// <summary>
	/// 登録されたインスタンスとその操作用関数を保持する型消去エントリ構造体
	/// </summary>
	struct RegistryEntry
	{
		// void* + カスタムデリータによる型消去ポインタ
		std::unique_ptr<void, void(*)(void*)> Ptr{ nullptr, [](void*) {} };

		std::function<void()>                  LoadFn;
		std::function<void()>                  DrawFn; // Releaseビルドでは nullptr
		std::string                            Label;
		bool                                   WindowOpen = true;
	};

	/// <summary>
	/// DataRegistry / ConfigRegistry が共有する型消去コンテナの基底クラス
	/// </summary>
	class RegistryBase
	{
	public:
		/// <summary>
		/// 指定された型が既に登録されているかどうかを判定します
		/// </summary>
		bool Contains(std::type_index key) const
		{
			return mEntries.count(key) > 0;
		}

	protected:
		/// <summary>
		/// 新しい型エントリを追加します（重複登録時はアサートが発生します）
		/// </summary>
		void AddEntry(std::type_index key, RegistryEntry entry)
		{
			assert(!Contains(key) && "[Registry] Type already registered.");
			mEntries.emplace(key, std::move(entry));
			mOrder.push_back(key);
		}

		/// <summary>
		/// 登録済みの型エントリへの参照を取得します（未登録時はアサートが発生します）
		/// </summary>
		RegistryEntry& GetEntry(std::type_index key)
		{
			auto it = mEntries.find(key);
			assert(it != mEntries.end() && "[Registry] Type not registered.");
			return it->second;
		}

		std::unordered_map<std::type_index, RegistryEntry> mEntries;
		std::vector<std::type_index>                       mOrder;
	};

} // namespace detail
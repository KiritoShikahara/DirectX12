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
    /// エントリ
    /// </summary>
    struct RegistryEntry
    {
        std::unique_ptr<void, void(*)(void*)> Ptr{ nullptr, [](void*) {} };
        std::function<void()>                 LoadFn;
        std::string                           Label;
    };

    /// <summary>
    /// レジストリの親
    /// Managerの一括管理をする
    /// </summary>
    class RegistryBase
    {
    public:
        bool Contains(std::type_index key) const { return mEntries.count(key) > 0; }

    protected:
        void AddEntry(std::type_index key, RegistryEntry entry)
        {
            assert(!Contains(key) && "[Registry] Type already registered.");
            mEntries.emplace(key, std::move(entry));
            mOrder.push_back(key);
        }

        RegistryEntry& GetEntry(std::type_index key)
        {
            auto it = mEntries.find(key);
            assert(it != mEntries.end() && "[Registry] Type not registered.");
            return it->second;
        }

        std::unordered_map<std::type_index, RegistryEntry> mEntries;
        std::vector<std::type_index>                       mOrder;
    };
}
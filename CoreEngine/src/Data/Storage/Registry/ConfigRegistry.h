#pragma once

#include"../Manager/ConfigManager.h"
#include"../Reflection.h"
#include"RegistryBase.h"
#include<Utility/Singleton/Singleton.hpp>
#include <unordered_map>
#include <typeindex>
#include <functional>

namespace data
{
    class ConfigRegistry
        : public utility::Singleton<ConfigRegistry>
        , private RegistryBase
    {
        SINGLETON_CLASS(ConfigRegistry);
    public:
        SINGLETON_ACCESSOR(ConfigRegistry);

        template<typename T>
        void Register(std::string filePath)
        {
            auto* raw = new ConfigManager<T>(std::move(filePath));

            RegistryEntry entry;
            entry.Ptr = { raw, [](void* p) { delete static_cast<ConfigManager<T>*>(p); } };
            entry.LoadFn = [raw]() { raw->Load(); };
            entry.Label = TypeDescriptor<T>::TableName();
            AddEntry(typeid(T), std::move(entry));

            mSaveFns[typeid(T)] = [raw]() { raw->Save(); };
        }

        void LoadAll()
        {
            for (const auto& key : mOrder)
                mEntries.at(key).LoadFn();
        }

        void SaveAll()
        {
            for (const auto& key : mOrder)
                mSaveFns.at(key)();
        }

        template<typename T>
        ConfigManager<T>& GetManager()
        {
            return *static_cast<ConfigManager<T>*>(GetEntry(typeid(T)).Ptr.get());
        }

        template<typename T>
        bool IsRegistered() const { return Contains(typeid(T)); }

        std::vector<std::string> GetLabels() const
        {
            std::vector<std::string> labels;
            for (const auto& key : mOrder)
                labels.push_back(mEntries.at(key).Label);
            return labels;
        }

    private:
        std::unordered_map<std::type_index, std::function<void()>> mSaveFns;
    };
}
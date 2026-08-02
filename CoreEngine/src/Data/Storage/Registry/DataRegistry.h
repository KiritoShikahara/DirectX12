#pragma once

#include"../Manager/DataManager.h"
#include"../Reflection.h"
#include"RegistryBase.h"
#include<Utility/Singleton/Singleton.hpp>

namespace data
{
    class DataRegistry
        : public utility::Singleton<DataRegistry>
        , private RegistryBase
    {
        SINGLETON_CLASS(DataRegistry);

    public:
        SINGLETON_ACCESSOR(DataRegistry);


        void Init(const std::string& dbPath)
        {
            mDb = std::make_shared<SqliteManager>(dbPath);
        }

        template<typename T>
        void Register(std::string csvPath)
        {
            assert(mDb && "[DataRegistry] Call Init() before Register().");

            // DataManager<T> には共有 DB を DI する
            auto* raw = new DataManager<T>(std::move(csvPath), mDb);

            RegistryEntry entry;
            entry.Ptr = { raw, [](void* p) { delete static_cast<DataManager<T>*>(p); } };
            entry.LoadFn = [raw]() { raw->Load(); };
            entry.Label = TypeDescriptor<T>::TableName();
            AddEntry(typeid(T), std::move(entry));
        }

        void LoadAll()
        {
            for (const auto& key : mOrder)
                mEntries.at(key).LoadFn();
        }

        template<typename T>
        DataManager<T>& GetManager()
        {
            return *static_cast<DataManager<T>*>(GetEntry(typeid(T)).Ptr.get());
        }

        template<typename T>
        bool IsRegistered() const { return Contains(typeid(T)); }

        // 登録順の Label リストを返す（Inspector 層が使う）
        std::vector<std::string> GetLabels() const
        {
            std::vector<std::string> labels;
            for (const auto& key : mOrder)
                labels.push_back(mEntries.at(key).Label);
            return labels;
        }

        std::shared_ptr<SqliteManager> GetDb() const { return mDb; }

    private:
        std::shared_ptr<SqliteManager> mDb;   // 全 DataManager<T> で共有
    };
}

#define DATA_MGR(Type) data::DataRegistry::Get().GetManager<Type>()
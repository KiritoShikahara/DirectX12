#pragma once

#include"../Reflection.h"
#include"../Loader/CsvParser.h"
#include"SqliteManager.h"
#include<string>
#include<vector>
#include<memory>
#include<functional>
#include<unordered_map>
#include<optional>
#include<stdexcept>

namespace data
{
    /// <summary>
    /// エラー用
    /// </summary>
    struct DuplicateKeyError : std::runtime_error
    {
        int DuplicateId;
        explicit DuplicateKeyError(int id)
            : std::runtime_error("[DataManager] Duplicate primary key: " + std::to_string(id))
            , DuplicateId(id) {
        }
    };

    /// <summary>
    /// PkExtractVisitor  （主キー値を int として取り出す）
    /// </summary>
    class PkExtractVisitor final : public IFieldVisitor
    {
    public:
        int  Value = 0;
        bool Found = false;
        void OnInt(const std::string&, int& v, eFieldFlag f) override { if (HasFlag(f, eFieldFlag::PrimaryKey)) { Value = v; Found = true; } }
        void OnFloat(const std::string&, float&, eFieldFlag)   override {}
        void OnBool(const std::string&, bool&, eFieldFlag)   override {}
        void OnString(const std::string&, std::string&, eFieldFlag)   override {}
    };

    template<typename T>
    class DataManager
    {
    public:
        DataManager(std::string csvPath, std::shared_ptr<SqliteManager> db)
            : mCsvPath(std::move(csvPath))
            , mDb(std::move(db))
        {
        }

        // 読み込み
        void Load()
        {
#ifdef _DEBUG
            LoadFromCsv();
#else
            LoadFromDb();
#endif
        }

        void LoadFromCsv()
        {
            mItems = CsvParser::Load<T>(mCsvPath);
            RebuildIndex();
            mLastMessage = "[CSV] Loaded " + std::to_string(mItems.size()) + " records.";
        }

        void LoadFromDb()
        {
            mDb->EnsureTable<T>();
            // 構造体にフィールドを追加した際、DB側の列が不足したままSELECTして
            // SQLite::Exceptionでクラッシュするのを防ぐ(SqliteManager::MigrateTable参照)
            mDb->MigrateTable<T>();
            mItems = mDb->LoadAll<T>();
            RebuildIndex();
            mLastMessage = "[DB] Loaded " + std::to_string(mItems.size()) + " records.";
        }

        void SaveCsvToDb()
        {
            RebuildIndex();
            mDb->SaveAll<T>(mItems);
            mLastMessage = "[DB] Saved " + std::to_string(mItems.size()) + " records.";
        }

        void SaveToCsv()
        {
            CsvParser::Save<T>(mCsvPath, mItems);
            mLastMessage = "[CSV] Saved " + std::to_string(mItems.size()) + " records.";
        }

        // アクセサ
        std::vector<T>& GetAll() { return mItems; }
        const std::vector<T>& GetAll() const { return mItems; }
        const std::string& GetCsvPath()   const { return mCsvPath; }
        const std::string& GetLastMessage() const { return mLastMessage; }

        T* Find(std::function<bool(const T&)> pred)
        {
            for (auto& item : mItems)
                if (pred(item)) return &item;
            return nullptr;
        }

        T* GetById(int id)
        {
            auto it = mIndexById.find(id);
            return it != mIndexById.end() ? it->second : nullptr;
        }
        const T* GetById(int id) const
        {
            auto it = mIndexById.find(id);
            return it != mIndexById.end() ? it->second : nullptr;
        }

        std::optional<T> FetchById(int id)
        {
            return mDb->template FetchById<T>(id);
        }

        void RebuildIndex()
        {
            mIndexById.clear();
            for (auto& item : mItems)
            {
                PkExtractVisitor vis;
                VisitFields(item, vis);
                if (!vis.Found) return;   // 主キー未登録の型はインデックスを作らない
                auto [it, inserted] = mIndexById.emplace(vis.Value, &item);
                if (!inserted) throw DuplicateKeyError(vis.Value);
            }
        }

    private:
        std::vector<T>                 mItems;
        std::unordered_map<int, T*>    mIndexById;
        std::string                    mCsvPath;
        std::shared_ptr<SqliteManager> mDb;          // 共有DB接続（DI）
        std::string                    mLastMessage; // 軽量なため Release でも保持
    };
}
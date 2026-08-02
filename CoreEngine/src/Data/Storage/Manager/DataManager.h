#pragma once

#include "../Reflection.h"
#include "../Loader/CsvParser.h"
#include "SqliteManager.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <optional>
#include <stdexcept>

namespace data
{
    /// <summary>
    /// 重複キーエラー
    /// </summary>
    struct DuplicateKeyError : std::runtime_error
    {
        /// <summary>重複したID</summary>
        int DuplicateId;

        explicit DuplicateKeyError(int id)
            : std::runtime_error("[DataManager] Duplicate primary key: " + std::to_string(id))
            , DuplicateId(id)
        {
        }
    };

    /// <summary>
    /// 主キー値を int として取り出すビジター
    /// </summary>
    class PkExtractVisitor final : public IFieldVisitor
    {
    public:
        /// <summary>主キーの値</summary>
        int  Value = 0;
        /// <summary>主キーが見つかったかどうか</summary>
        bool Found = false;

        void OnInt(const std::string&, int& v, eFieldFlag f) override
        {
            if (HasFlag(f, eFieldFlag::PrimaryKey))
            {
                Value = v;
                Found = true;
            }
        }
        void OnFloat(const std::string&, float&, eFieldFlag)   override {}
        void OnBool(const std::string&, bool&, eFieldFlag)   override {}
        void OnString(const std::string&, std::string&, eFieldFlag)   override {}
    };

    /// <summary>
    /// データマネージャー
    /// </summary>
    template<typename T>
    class DataManager
    {
    public:
        DataManager(std::string csvPath, std::shared_ptr<SqliteManager> db)
            : mCsvPath(std::move(csvPath))
            , mDb(std::move(db))
        {
        }

        /// <summary>データをロードする</summary>
        void Load()
        {
#ifdef _DEBUG
            LoadFromCsv();
#else
            LoadFromDb();
#endif
        }

        /// <summary>CSVからデータをロードする</summary>
        void LoadFromCsv()
        {
            mItems = CsvParser::Load<T>(mCsvPath);
            RebuildIndex();
            mLastMessage = "[CSV] Loaded " + std::to_string(mItems.size()) + " records.";
        }

        /// <summary>データベースからデータをロードする</summary>
        void LoadFromDb()
        {
            mDb->EnsureTable<T>();
            mDb->MigrateTable<T>();
            mItems = mDb->LoadAll<T>();
            RebuildIndex();
            mLastMessage = "[DB] Loaded " + std::to_string(mItems.size()) + " records.";
        }

        /// <summary>CSVのデータをデータベースに保存する</summary>
        void SaveCsvToDb()
        {
            RebuildIndex();
            mDb->SaveAll<T>(mItems);
            mLastMessage = "[DB] Saved " + std::to_string(mItems.size()) + " records.";
        }

        /// <summary>CSVにデータを保存する</summary>
        void SaveToCsv()
        {
            CsvParser::Save<T>(mCsvPath, mItems);
            mLastMessage = "[CSV] Saved " + std::to_string(mItems.size()) + " records.";
        }

        /// <summary>全データを取得する（非同期編集用）</summary>
        std::vector<T>& GetAll() { return mItems; }

        /// <summary>全データを取得する（読み取り専用）</summary>
        const std::vector<T>& GetAll() const { return mItems; }

        /// <summary>CSVのファイルパスを取得する</summary>
        const std::string& GetCsvPath()   const { return mCsvPath; }

        /// <summary>直近のメッセージを取得する</summary>
        const std::string& GetLastMessage() const { return mLastMessage; }

        /// <summary>条件に一致する最初の要素を検索する</summary>
        T* Find(std::function<bool(const T&)> pred)
        {
            for (auto& item : mItems)
                if (pred(item)) return &item;
            return nullptr;
        }

        /// <summary>IDを指定して要素を取得する</summary>
        T* GetById(int id)
        {
            auto it = mIndexById.find(id);
            return it != mIndexById.end() ? it->second : nullptr;
        }

        /// <summary>IDを指定して要素を取得する（読み取り専用）</summary>
        const T* GetById(int id) const
        {
            auto it = mIndexById.find(id);
            return it != mIndexById.end() ? it->second : nullptr;
        }

        /// <summary>データベースからIDを指定して要素を取得する</summary>
        std::optional<T> FetchById(int id)
        {
            return mDb->template FetchById<T>(id);
        }

        /// <summary>インデックスを再構築する</summary>
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
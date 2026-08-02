#pragma once
#include "../Reflection.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <stdexcept>
#include <algorithm>

namespace data
{
    /// <summary>
    /// SQL型判定用
    /// </summary>
    enum class eFieldType { Int, Float, Bool, String };

    /// <summary>
    /// SQLite読み込み用ビジター
    /// </summary>
    class SqliteReadVisitor final : public IFieldVisitor
    {
    public:
        explicit SqliteReadVisitor(SQLite::Statement& stmt) : mStmt(stmt) {}
        void OnInt(const std::string&, int& v, eFieldFlag) override { v = mStmt.getColumn(mCol++).getInt(); }
        void OnFloat(const std::string&, float& v, eFieldFlag) override { v = static_cast<float>(mStmt.getColumn(mCol++).getDouble()); }
        void OnBool(const std::string&, bool& v, eFieldFlag) override { v = (mStmt.getColumn(mCol++).getInt() != 0); }
        void OnString(const std::string&, std::string& v, eFieldFlag) override { v = mStmt.getColumn(mCol++).getString(); }
    private:
        SQLite::Statement& mStmt;
        int                mCol = 0;
    };

    /// <summary>
    /// SQLite書き込み用ビジター
    /// </summary>
    class SqliteWriteVisitor final : public IFieldVisitor
    {
    public:
        explicit SqliteWriteVisitor(SQLite::Statement& stmt) : mStmt(stmt) {}
        void OnInt(const std::string&, int& v, eFieldFlag) override { mStmt.bind(mIdx++, v); }
        void OnFloat(const std::string&, float& v, eFieldFlag) override { mStmt.bind(mIdx++, static_cast<double>(v)); }
        void OnBool(const std::string&, bool& v, eFieldFlag) override { mStmt.bind(mIdx++, v ? 1 : 0); }
        void OnString(const std::string&, std::string& v, eFieldFlag) override { mStmt.bind(mIdx++, v); }
    private:
        SQLite::Statement& mStmt;
        int                mIdx = 1;
    };

    /// <summary>
    /// SQL 型文字列を収集するビジター
    /// </summary>
    class SqlTypeVisitor final : public IFieldVisitor
    {
    public:
        /// <summary>SQL型のリスト</summary>
        std::vector<std::string> Types;
        void OnInt(const std::string&, int&, eFieldFlag) override { Types.push_back("INTEGER"); }
        void OnFloat(const std::string&, float&, eFieldFlag) override { Types.push_back("REAL"); }
        void OnBool(const std::string&, bool&, eFieldFlag) override { Types.push_back("INTEGER"); }
        void OnString(const std::string&, std::string&, eFieldFlag) override { Types.push_back("TEXT"); }
    };

    /// <summary>
    /// 列定義を収集するビジター
    /// </summary>
    class SqlColumnDefVisitor final : public IFieldVisitor
    {
    public:
        /// <summary>カラム構造体</summary>
        struct Column
        {
            /// <summary>カラム名</summary>
            std::string Name;
            /// <summary>SQL型</summary>
            std::string Type;
            /// <summary>既定値リテラル</summary>
            std::string DefaultLiteral;
        };
        /// <summary>カラム定義のリスト</summary>
        std::vector<Column> Columns;

        void OnInt(const std::string& n, int& v, eFieldFlag) override
        {
            Columns.push_back({ n, "INTEGER", std::to_string(v) });
        }
        void OnFloat(const std::string& n, float& v, eFieldFlag) override
        {
            Columns.push_back({ n, "REAL", std::to_string(v) });
        }
        void OnBool(const std::string& n, bool& v, eFieldFlag) override
        {
            Columns.push_back({ n, "INTEGER", v ? "1" : "0" });
        }
        void OnString(const std::string& n, std::string& v, eFieldFlag) override
        {
            Columns.push_back({ n, "TEXT", "'" + EscapeSqlLiteral(v) + "'" });
        }

    private:
        /// <summary>SQL文字列リテラル用に単一引用符をエスケープする</summary>
        static std::string EscapeSqlLiteral(const std::string& value)
        {
            std::string escaped;
            escaped.reserve(value.size());
            for (char c : value)
            {
                if (c == '\'') escaped += '\'';
                escaped += c;
            }
            return escaped;
        }
    };

    /// <summary>
    /// 主キー名を取り出すビジター
    /// </summary>
    class PkNameVisitor final : public IFieldVisitor
    {
    public:
        /// <summary>主キー名</summary>
        std::string PkName;
        void OnInt(const std::string& name, int&, eFieldFlag f) override { if (HasFlag(f, eFieldFlag::PrimaryKey)) PkName = name; }
        void OnFloat(const std::string&, float&, eFieldFlag)   override {}
        void OnBool(const std::string&, bool&, eFieldFlag)   override {}
        void OnString(const std::string&, std::string&, eFieldFlag)   override {}
    };

    /// <summary>
    /// Sqliteの管理をするクラス
    /// </summary>
    class SqliteManager
    {
    public:
        explicit SqliteManager(const std::string& dbPath)
            : mDb(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
        {
        }

        /// <summary>テーブルが無ければ作成する</summary>
        template<typename T>
        void EnsureTable()
        {
            const auto& fields = TypeDescriptor<T>::Fields();
            const char* table = TypeDescriptor<T>::TableName();

            T dummy{};
            SqlTypeVisitor typeVis;
            VisitFields(dummy, typeVis);

            std::string sql = "CREATE TABLE IF NOT EXISTS ";
            sql += table; sql += " (";
            for (int i = 0; i < (int)fields.size(); ++i)
            {
                if (i > 0) sql += ", ";
                sql += fields[i].Name; sql += " "; sql += typeVis.Types[i];
                if (HasFlag(fields[i].Flags, eFieldFlag::PrimaryKey)) sql += " PRIMARY KEY";
            }
            sql += ");";
            mDb.exec(sql);
        }

        /// <summary>既存テーブルに不足している列を追加する</summary>
        template<typename T>
        void MigrateTable()
        {
            const char* table = TypeDescriptor<T>::TableName();

            std::vector<std::string> existingColumns;
            {
                SQLite::Statement stmt(mDb, std::string("PRAGMA table_info(") + table + ");");
                while (stmt.executeStep())
                {
                    existingColumns.push_back(stmt.getColumn(1).getString());
                }
            }

            if (existingColumns.empty()) return;

            T dummy{};
            SqlColumnDefVisitor visitor;
            VisitFields(dummy, visitor);

            for (const auto& column : visitor.Columns)
            {
                const bool exists = std::find(
                    existingColumns.begin(), existingColumns.end(), column.Name) != existingColumns.end();
                if (exists) continue;

                mDb.exec("ALTER TABLE " + std::string(table) + " ADD COLUMN "
                    + column.Name + " " + column.Type + " DEFAULT " + column.DefaultLiteral + ";");
            }
        }

        /// <summary>テーブルを削除する</summary>
        template<typename T>
        void DropTable()
        {
            const char* table = TypeDescriptor<T>::TableName();
            mDb.exec(std::string("DROP TABLE IF EXISTS ") + table + ";");
        }

        /// <summary>全データを読み込む</summary>
        template<typename T>
        std::vector<T> LoadAll()
        {
            const auto& fields = TypeDescriptor<T>::Fields();
            SQLite::Statement stmt(mDb, BuildSelectAll(fields, TypeDescriptor<T>::TableName()));
            std::vector<T> result;
            while (stmt.executeStep())
            {
                T item{};
                SqliteReadVisitor visitor(stmt);
                VisitFields(item, visitor);
                result.push_back(std::move(item));
            }
            return result;
        }

        /// <summary>IDを指定してデータを取得する</summary>
        template<typename T>
        std::optional<T> FetchById(int id)
        {
            const auto& fields = TypeDescriptor<T>::Fields();
            const char* table = TypeDescriptor<T>::TableName();

            T dummy{};
            PkNameVisitor pkVis;
            VisitFields(dummy, pkVis);
            if (pkVis.PkName.empty()) return std::nullopt;

            std::string sql = BuildSelectAll(fields, table);
            sql += " WHERE "; sql += pkVis.PkName; sql += " = ?;";

            SQLite::Statement stmt(mDb, sql);
            stmt.bind(1, id);
            if (!stmt.executeStep()) return std::nullopt;

            T item{};
            SqliteReadVisitor visitor(stmt);
            VisitFields(item, visitor);
            return item;
        }

        /// <summary>全データを保存する</summary>
        template<typename T>
        void SaveAll(const std::vector<T>& items)
        {
            const auto& fields = TypeDescriptor<T>::Fields();
            const char* table = TypeDescriptor<T>::TableName();

            EnsureTable<T>();
            SQLite::Transaction tx(mDb);
            mDb.exec(std::string("DELETE FROM ") + table + ";");

            std::string sql = "INSERT INTO "; sql += table; sql += " (";
            for (int i = 0; i < (int)fields.size(); ++i) { if (i > 0) sql += ", "; sql += fields[i].Name; }
            sql += ") VALUES (";
            for (int i = 0; i < (int)fields.size(); ++i) { if (i > 0) sql += ", "; sql += "?"; }
            sql += ");";

            SQLite::Statement stmt(mDb, sql);
            for (const T& item : items)
            {
                stmt.reset();
                SqliteWriteVisitor visitor(stmt);
                VisitFields(const_cast<T&>(item), visitor);
                stmt.exec();
            }
            tx.commit();
        }

        /// <summary>データベースインスタンスを取得する</summary>
        SQLite::Database& GetDb() { return mDb; }

    private:
        /// <summary>データベース</summary>
        SQLite::Database mDb;

        /// <summary>SELECT文を構築する</summary>
        template<typename FieldList>
        static std::string BuildSelectAll(const FieldList& fields, const char* table)
        {
            std::string sql = "SELECT ";
            for (int i = 0; i < (int)fields.size(); ++i) { if (i > 0) sql += ", "; sql += fields[i].Name; }
            sql += " FROM "; sql += table;
            return sql;
        }
    };
}
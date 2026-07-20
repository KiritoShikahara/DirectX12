#pragma once
#include"../Reflection.h"
#include<SQLiteCpp/SQLiteCpp.h>
#include<string>
#include<vector>
#include<optional>
#include<memory>
#include<stdexcept>
#include<algorithm>

namespace data
{
	// SQL型判定用
	enum class eFieldType { Int, Float, Bool, String };

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

    // SQL 型文字列を収集するビジター
    class SqlTypeVisitor final : public IFieldVisitor
    {
    public:
        std::vector<std::string> Types;
        void OnInt(const std::string&, int&, eFieldFlag) override { Types.push_back("INTEGER"); }
        void OnFloat(const std::string&, float&, eFieldFlag) override { Types.push_back("REAL"); }
        void OnBool(const std::string&, bool&, eFieldFlag) override { Types.push_back("INTEGER"); }
        void OnString(const std::string&, std::string&, eFieldFlag) override { Types.push_back("TEXT"); }
    };

    /// <summary>
    /// 列定義(名前・SQL型・既定値リテラル)を収集するビジター。
    /// 既定値はデフォルト構築したインスタンスの値をそのまま使うため、
    /// 後から追加した列にも構造体と同じ初期値が入る。
    /// </summary>
    class SqlColumnDefVisitor final : public IFieldVisitor
    {
    public:
        struct Column
        {
            std::string Name;
            std::string Type;
            std::string DefaultLiteral;
        };
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

    // 主キー名を取り出すビジター
    class PkNameVisitor final : public IFieldVisitor
    {
    public:
        std::string PkName;
        void OnInt(const std::string& name, int&, eFieldFlag f) override { if (HasFlag(f, eFieldFlag::PrimaryKey)) PkName = name; }
        void OnFloat(const std::string&, float&, eFieldFlag)   override {}
        void OnBool(const std::string&, bool&, eFieldFlag)   override {}
        void OnString(const std::string&, std::string&, eFieldFlag)   override {}
    };

    /// <summary>
    /// Sqliteの管理をする
    /// １インスタンスをShaderedで共有する方向で。
    /// </summary>
    class SqliteManager
    {
    public:
        explicit SqliteManager(const std::string& dbPath)
            : mDb(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
        {
        }

        // テーブルが無ければ作成
        template<typename T>
        void EnsureTable()
        {
            const auto& fields = TypeDescriptor<T>::Fields();
            const char* table = TypeDescriptor<T>::TableName();

            // SQL 型を収集するためダミーインスタンスで VisitFields
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

        /// <summary>
        /// 既存テーブルに不足している列を追加する(前方互換のマイグレーション)。
        ///
        /// EnsureTable()のCREATE TABLE IF NOT EXISTSは既存テーブルの列構成を更新しないため、
        /// 構造体へフィールドを1つ追加しただけで、以降LoadAll<T>()のSELECTが
        /// 「no such column」でSQLite::Exceptionを投げ、Releaseビルドが起動直後に
        /// クラッシュする(CSVを直接読むDebugビルドでは再現しない)という事故が起きる。
        /// これを構造的に防ぐため、読み込み前に構造体の定義とDBの実列を突き合わせ、
        /// 不足分をALTER TABLEで補う。
        ///
        /// 追加した列には構造体のデフォルト値が入る(既存行にも適用される)。
        /// 列の削除・リネームは扱わない(SELECTは既知の列しか要求しないため、
        /// DB側に余分な列が残っていても実害がない)。
        /// </summary>
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

            // テーブル自体が存在しない場合はEnsureTable()が正しい構成で作るため何もしない
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

        // テーブルを削除する。CREATE TABLE IF NOT EXISTS(EnsureTable)は既存テーブルの列構成を
        // 更新しないため、フィールド追加/削除等のスキーマ変更時はこれで一度削除してから
        // SaveAll<T>()等を呼び、新しい列構成で作り直す
        template<typename T>
        void DropTable()
        {
            const char* table = TypeDescriptor<T>::TableName();
            mDb.exec(std::string("DROP TABLE IF EXISTS ") + table + ";");
        }

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

        SQLite::Database& GetDb() { return mDb; }

    private:
        SQLite::Database mDb;

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
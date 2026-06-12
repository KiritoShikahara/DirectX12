#pragma once

#include"DataReflection.h"
#include<SQLiteCpp/SQLiteCpp.h>

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <stdexcept>


namespace data
{
	class SqliteManager
	{
	public:

		/// <summary>
		/// データベースの初期化
		/// </summary>
		bool Initialize(const std::string& dbPath);

		/// <summary>
		/// テーブルがなければ作成
		/// PrimaryKeyフラグが立っているフィールドは PRIMARY KEY として定義
		/// </summary>
		template<typename T>
		void EnsureTable();

		/// <summary>
		/// 全件読み込み
		/// </summary>
		template<typename T>
		std::vector<T> LoadAll();

		/// <summary>
		/// オンデマンド取得（主キーで1件取得
		/// 省メモリ運用向け：データを全件 mItems に載せず、必要な時だけ DB に問い合わせる
		/// 主キーフィールド（REFLECT_FIELD_ID）が登録されていない型では std::nullopt を返す
		/// </summary>
		template<typename T>
		std::optional<T> FetchById(int id);

		/// <summary>
		/// 全件書き込み
		/// </summary>
		template<typename T>
		void SaveAll(const std::vector<T>& items);

		SQLite::Database& GetDb() { return *mDb; }
	private:
		/// <summary>
		/// 
		/// </summary>
		template<typename FieldList>
		static std::string BuildSelectAll(const FieldList& fields, const char* table);

		static const char* SqlType(reflect::eFieldType type);

		static void BindFromQuery(SQLite::Statement& q, int col,
			reflect::FieldValue& ptr);

		static void BindToStatement(SQLite::Statement& stmt, int idx,
			reflect::FieldValue& ptr);
	private:
		std::unique_ptr<SQLite::Database> mDb;
	};

	template<typename T>
	inline void SqliteManager::EnsureTable()
	{
		const auto& fields = reflect::TypeDescriptor<T>::Fields();
		const char* table = reflect::TypeDescriptor<T>::TableName();

		std::string sql = "CREATE TABLE IF NOT EXISTS ";
		sql += table;
		sql += " (";

		for (int i = 0; i < (int)fields.size(); ++i)
		{
			if (i > 0) sql += ", ";
			sql += fields[i].Name;
			sql += " ";
			sql += SqlType(fields[i].Type);

			if (reflect::HasFlag(fields[i].Flags, reflect::eFieldFlag::PrimaryKey))
				sql += " PRIMARY KEY";
		}
		sql += ");";

		mDb->exec(sql);
	}

	template<typename T>
	inline std::vector<T> SqliteManager::LoadAll()
	{
		const auto& fields = reflect::TypeDescriptor<T>::Fields();
		const char* table = reflect::TypeDescriptor<T>::TableName();

		SQLite::Statement query(*mDb, BuildSelectAll(fields, table));
		std::vector<T> result;

		while (query.executeStep())
		{
			T item{};
			for (int col = 0; col < (int)fields.size(); ++col)
			{
				reflect::FieldValue ptr = fields[col].GetPtr(&item);
				BindFromQuery(query, col, ptr);
			}
			result.push_back(std::move(item));
		}

		return result;
	}

	template<typename T>
	inline std::optional<T> SqliteManager::FetchById(int id)
	{
		const auto& fields = reflect::TypeDescriptor<T>::Fields();
		const char* table = reflect::TypeDescriptor<T>::TableName();

		// 主キーフィールドを探す
		int pkCol = -1;
		for (int i = 0; i < (int)fields.size(); ++i)
		{
			if (reflect::HasFlag(fields[i].Flags, reflect::eFieldFlag::PrimaryKey))
			{
				pkCol = i;
				break;
			}
		}
		if (pkCol < 0) return std::nullopt;

		std::string sql = BuildSelectAll(fields, table);
		sql += " WHERE ";
		sql += fields[pkCol].Name;
		sql += " = ?;";

		SQLite::Statement query(*mDb, sql);
		query.bind(1, id);

		if (!query.executeStep()) return std::nullopt;

		T item{};
		for (int col = 0; col < (int)fields.size(); ++col)
		{
			reflect::FieldValue ptr = fields[col].GetPtr(&item);
			BindFromQuery(query, col, ptr);
		}
		return item;
	}

	template<typename T>
	inline void SqliteManager::SaveAll(const std::vector<T>& items)
	{
		const auto& fields = reflect::TypeDescriptor<T>::Fields();
		const char* table = reflect::TypeDescriptor<T>::TableName();

		EnsureTable<T>();

		SQLite::Transaction transaction(*mDb);

		mDb->exec(std::string("DELETE FROM ") + table + ";");

		// INSERT 文構築
		std::string sql = "INSERT INTO ";
		sql += table;
		sql += " (";
		for (int i = 0; i < (int)fields.size(); ++i)
		{
			if (i > 0) sql += ", ";
			sql += fields[i].Name;
		}
		sql += ") VALUES (";
		for (int i = 0; i < (int)fields.size(); ++i)
		{
			if (i > 0) sql += ", ";
			sql += "?";
		}
		sql += ");";

		SQLite::Statement stmt(*mDb, sql);

		for (const T& item : items)
		{
			stmt.reset();
			for (int col = 0; col < (int)fields.size(); ++col)
			{
				reflect::FieldValue ptr = fields[col].GetPtr(const_cast<T*>(&item));
				BindToStatement(stmt, col + 1, ptr);  // SQLiteCpp は 1-origin
			}
			stmt.exec();
		}

		transaction.commit();
	}

	template<typename FieldList>
	inline std::string SqliteManager::BuildSelectAll(const FieldList& fields, const char* table)
	{
		std::string sql = "SELECT ";
		for (int i = 0; i < (int)fields.size(); ++i)
		{
			if (i > 0) sql += ", ";
			sql += fields[i].Name;
		}
		sql += " FROM ";
		sql += table;
		return sql;
	}
}
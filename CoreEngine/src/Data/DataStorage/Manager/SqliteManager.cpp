#include "pch.h"
#include "SqliteManager.h"

namespace data
{
	bool SqliteManager::Initialize(const std::string& dbPath)
	{
		mDb = std::make_unique<SQLite::Database>(
			dbPath,
			SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
		return true;
	}

	const char* SqliteManager::SqlType(reflect::eFieldType type)
	{
		switch (type)
		{
		case reflect::eFieldType::Int:    return "INTEGER";
		case reflect::eFieldType::Float:  return "REAL";
		case reflect::eFieldType::Bool:   return "INTEGER";   // 0/1
		case reflect::eFieldType::String: return "TEXT";
		default:                          return "TEXT";
		}
	}

	void SqliteManager::BindFromQuery(SQLite::Statement& q, int col, reflect::FieldValue& ptr)
	{
		std::visit([&](auto* p)
			{
				using P = std::decay_t<decltype(*p)>;
				if constexpr (std::is_same_v<P, int>)         *p = q.getColumn(col).getInt();
				else if constexpr (std::is_same_v<P, float>)       *p = static_cast<float>(q.getColumn(col).getDouble());
				else if constexpr (std::is_same_v<P, bool>)        *p = (q.getColumn(col).getInt() != 0);
				else if constexpr (std::is_same_v<P, std::string>) *p = q.getColumn(col).getString();
			}, ptr);
	}
	void SqliteManager::BindToStatement(SQLite::Statement& stmt, int idx, reflect::FieldValue& ptr)
	{
		std::visit([&](auto* p)
			{
				using P = std::decay_t<decltype(*p)>;
				if constexpr (std::is_same_v<P, int>)         stmt.bind(idx, *p);
				else if constexpr (std::is_same_v<P, float>)       stmt.bind(idx, static_cast<double>(*p));
				else if constexpr (std::is_same_v<P, bool>)        stmt.bind(idx, *p ? 1 : 0);
				else if constexpr (std::is_same_v<P, std::string>) stmt.bind(idx, *p);
			}, ptr);
	}
}

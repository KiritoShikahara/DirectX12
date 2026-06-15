#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace data
{
	/*
	* フィールド属性フラグ
	*/
	enum class eFieldFlag : uint32_t
	{
		None = 0,
		PrimaryKey = 1 << 0,
	};
	inline eFieldFlag operator|(eFieldFlag a, eFieldFlag b)
	{
		return static_cast<eFieldFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}
	inline bool HasFlag(eFieldFlag flags, eFieldFlag bit)
	{
		return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(bit)) != 0;
	}

	/// <summary>
	/// 各システム登録用
	/// </summary>
	class IFieldVisitor
	{
	public:
		virtual ~IFieldVisitor() = default;
		virtual void OnInt(const std::string& name, int& value, eFieldFlag flags) = 0;
		virtual void OnFloat(const std::string& name, float& value, eFieldFlag flags) = 0;
		virtual void OnBool(const std::string& name, bool& value, eFieldFlag flags) = 0;
		virtual void OnString(const std::string& name, std::string& value, eFieldFlag flags) = 0;
		// 将来の型拡張例：
		// virtual void OnVec3(const std::string& name, Vector3& value, eFieldFlag flags) {}
	};

	struct FieldInfo
	{
		std::string Name;
		eFieldFlag  Flags = eFieldFlag::None;
		std::function<void(void* inst, IFieldVisitor& visitor)> Accept;
	};

	/// <summary>
	/// TypeDescriptor<T>  マクロで特殊化する
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<typename T>
	struct TypeDescriptor
	{
		static const char* TableName();
		static const std::vector<FieldInfo>& Fields();
	};

	/*
	* VisitFields  ユーティリティ
	* TypeDescriptor<T>::Fields() を回してビジターを適用する。
	*/
	template<typename T>
		void VisitFields(T& obj, IFieldVisitor& visitor)
	{
		for (const auto& field : TypeDescriptor<T>::Fields())
			field.Accept(&obj, visitor);
	}

	template<typename T>
	void VisitFields(const T& obj, IFieldVisitor& visitor)
	{
		// const → mutable キャスト（visitor は読み取り専用として使う場合）
		for (const auto& field : TypeDescriptor<T>::Fields())
			field.Accept(const_cast<T*>(&obj), visitor);
	}

	// 主キーフィールドのインデックスを返す（無ければ -1）
	template<typename T>
	int FindPrimaryKeyIndex()
	{
		const auto& fields = TypeDescriptor<T>::Fields();
		for (int i = 0; i < (int)fields.size(); ++i)
			if (HasFlag(fields[i].Flags, eFieldFlag::PrimaryKey)) return i;
		return -1;
	}

	template<typename F>
	inline void InvokeVisitor(const std::string& name, F& field, IFieldVisitor& visitor, eFieldFlag flags)
	{
		if constexpr (std::is_same_v<F, int>)         visitor.OnInt(name, field, flags);
		else if constexpr (std::is_same_v<F, float>)       visitor.OnFloat(name, field, flags);
		else if constexpr (std::is_same_v<F, bool>)        visitor.OnBool(name, field, flags);
		else if constexpr (std::is_same_v<F, std::string>) visitor.OnString(name, field, flags);
	}
}


#define REFLECT_BEGIN(Type, TableNameStr)                                                \
	static constexpr const char* kReflectTableName = TableNameStr;                      \
	static const std::vector<data::FieldInfo>& GetReflectFields()                       \
	{                                                                                    \
		using _T = Type;                                                                 \
		static std::vector<data::FieldInfo> sFields = {

// 通常フィールド（型自動判別）


#define REFLECT_FIELD(FieldName)                                                         \
			data::FieldInfo {                                                            \
				#FieldName,                                                              \
				data::eFieldFlag::None,                                                  \
				[](void* inst, data::IFieldVisitor& v)                                  \
				{                                                                        \
					auto& self = *static_cast<_T*>(inst);                               \
					using F = decltype(self.FieldName);                                  \
					if constexpr      (std::is_same_v<F, int>)         v.OnInt   (#FieldName, self.FieldName, data::eFieldFlag::None); \
					else if constexpr (std::is_same_v<F, float>)       v.OnFloat (#FieldName, self.FieldName, data::eFieldFlag::None); \
					else if constexpr (std::is_same_v<F, bool>)        v.OnBool  (#FieldName, self.FieldName, data::eFieldFlag::None); \
					else if constexpr (std::is_same_v<F, std::string>) v.OnString(#FieldName, self.FieldName, data::eFieldFlag::None); \
				}                                                                        \
			},

// 主キーフィールド（int 専用）
#define REFLECT_FIELD_ID(FieldName)                                                      \
			data::FieldInfo {                                                            \
				#FieldName,                                                              \
				data::eFieldFlag::PrimaryKey,                                            \
				[](void* inst, data::IFieldVisitor& v)                                  \
				{                                                                        \
					auto& self = *static_cast<_T*>(inst);                               \
					v.OnInt(#FieldName, self.FieldName, data::eFieldFlag::PrimaryKey);   \
				}                                                                        \
			},

// 型別明示マクロ（型推論が効かないケース用）
#define REFLECT_FIELD_INT(FieldName)                                                     \
			data::FieldInfo { #FieldName, data::eFieldFlag::None,                        \
				[](void* inst, data::IFieldVisitor& v) {                                 \
					v.OnInt(#FieldName, static_cast<_T*>(inst)->FieldName, data::eFieldFlag::None); } },

#define REFLECT_FIELD_FLOAT(FieldName)                                                   \
			data::FieldInfo { #FieldName, data::eFieldFlag::None,                        \
				[](void* inst, data::IFieldVisitor& v) {                                 \
					v.OnFloat(#FieldName, static_cast<_T*>(inst)->FieldName, data::eFieldFlag::None); } },

#define REFLECT_FIELD_BOOL(FieldName)                                                    \
			data::FieldInfo { #FieldName, data::eFieldFlag::None,                        \
				[](void* inst, data::IFieldVisitor& v) {                                 \
					v.OnBool(#FieldName, static_cast<_T*>(inst)->FieldName, data::eFieldFlag::None); } },

#define REFLECT_FIELD_STR(FieldName)                                                     \
			data::FieldInfo { #FieldName, data::eFieldFlag::None,                        \
				[](void* inst, data::IFieldVisitor& v) {                                 \
					v.OnString(#FieldName, static_cast<_T*>(inst)->FieldName, data::eFieldFlag::None); } },

#define REFLECT_END()                                                                    \
		};                                                                               \
		return sFields;                                                                  \
	}

// TypeDescriptor 特殊化（構造体定義直後・名前空間外）
#define REFLECT_REGISTER(Type)                                                           \
	namespace data {                                                                     \
	template<> inline const char* TypeDescriptor<Type>::TableName()                     \
		{ return Type::kReflectTableName; }                                              \
	template<> inline const std::vector<FieldInfo>& TypeDescriptor<Type>::Fields()      \
		{ return Type::GetReflectFields(); }                                             \
	}
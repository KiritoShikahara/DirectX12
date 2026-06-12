#pragma once

#include<string>
#include<vector>
#include<functional>
#include<variant>
#include<cstdint>

namespace reflect
{
    using FieldValue = std::variant<
        int*,
        float*,
        bool*,
        std::string*
    >;

    enum class eFieldType { Int, Float, Bool, String };

    /// <summary>
    /// フィールド属性フラグ
    /// </summary>
    enum class eFieldFlag : uint32_t
    {
        None = 0,
        PrimaryKey = 1 << 0,   // REFLECT_FIELD_ID で付与 → 重複チェック対象
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
    /// 1フィールドの情報
    /// </summary>
    struct FieldInfo
    {
        std::string     Name;
        eFieldType      Type;
        eFieldFlag      Flags = eFieldFlag::None;

        // インスタンスポインタからフィールドポインタを取得するアクセサ
        std::function<FieldValue(void*)>  GetPtr;
    };

    /// <summary>
    /// 型ごとのフィールド情報
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template<typename T>
    struct TypeDescriptor
    {
        static const char* TableName();
        static const std::vector<FieldInfo>& Fields();
    };

}

// 登録マクロ
#define REFLECT_BEGIN(Type, TableNameStr)                                        \
    static constexpr const char* kReflectTableName = TableNameStr;              \
    static const std::vector<reflect::FieldInfo>& GetReflectFields()            \
    {                                                                            \
        using _T = Type;                                                         \
        static std::vector<reflect::FieldInfo> sFields = {

// 型別マクロ
#define REFLECT_FIELD_INT(FieldName)                                             \
            reflect::FieldInfo {                                                 \
                #FieldName, reflect::eFieldType::Int, reflect::eFieldFlag::None, \
                [](void* inst) -> reflect::FieldValue {                         \
                    return &static_cast<_T*>(inst)->FieldName; }                 \
            },

#define REFLECT_FIELD_FLOAT(FieldName)                                           \
            reflect::FieldInfo {                                                 \
                #FieldName, reflect::eFieldType::Float, reflect::eFieldFlag::None, \
                [](void* inst) -> reflect::FieldValue {                         \
                    return &static_cast<_T*>(inst)->FieldName; }                 \
            },

#define REFLECT_FIELD_BOOL(FieldName)                                            \
            reflect::FieldInfo {                                                 \
                #FieldName, reflect::eFieldType::Bool, reflect::eFieldFlag::None, \
                [](void* inst) -> reflect::FieldValue {                         \
                    return &static_cast<_T*>(inst)->FieldName; }                 \
            },

#define REFLECT_FIELD_STR(FieldName)                                             \
            reflect::FieldInfo {                                                 \
                #FieldName, reflect::eFieldType::String, reflect::eFieldFlag::None, \
                [](void* inst) -> reflect::FieldValue {                         \
                    return &static_cast<_T*>(inst)->FieldName; }                 \
            },

// 型を自動判別する汎用マクロ
#define REFLECT_FIELD(FieldName)                                                 \
            reflect::FieldInfo {                                                 \
                #FieldName,                                                      \
                []() -> reflect::eFieldType {                                    \
                    using F = decltype(_T::FieldName);                           \
                    if constexpr      (std::is_same_v<F, int>)   return reflect::eFieldType::Int;   \
                    else if constexpr (std::is_same_v<F, float>) return reflect::eFieldType::Float; \
                    else if constexpr (std::is_same_v<F, bool>)  return reflect::eFieldType::Bool;  \
                    else                                          return reflect::eFieldType::String;\
                }(),                                                             \
                reflect::eFieldFlag::None,                                       \
                [](void* inst) -> reflect::FieldValue {                         \
                    return &static_cast<_T*>(inst)->FieldName; }                 \
            },

// 主キーフィールド（int のみ対応）
// DataManager の重複チェック・GetById 高速検索の対象になる
#define REFLECT_FIELD_ID(FieldName)                                              \
            reflect::FieldInfo {                                                 \
                #FieldName,                                                      \
                reflect::eFieldType::Int,                                        \
                reflect::eFieldFlag::PrimaryKey,                                 \
                [](void* inst) -> reflect::FieldValue {                         \
                    return &static_cast<_T*>(inst)->FieldName; }                 \
            },

#define REFLECT_END()                                                            \
        };                                                                       \
        return sFields;                                                          \
    }

// TypeDescriptor 特殊化マクロ
#define REFLECT_REGISTER(Type)                                                   \
    namespace reflect {                                                          \
    template<> const char* TypeDescriptor<Type>::TableName()                    \
        { return Type::kReflectTableName; }                                      \
    template<> const std::vector<FieldInfo>& TypeDescriptor<Type>::Fields()     \
        { return Type::GetReflectFields(); }                                     \
    }

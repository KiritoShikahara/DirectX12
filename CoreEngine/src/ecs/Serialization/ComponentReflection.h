#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <functional>
#include <type_traits>

namespace ecs
{
	/// <summary>
	/// ECS コンポーネントのフィールドを走査するビジターインターフェース
	/// </summary>
	class IComponentFieldVisitor
	{
	public:
		virtual ~IComponentFieldVisitor() = default;

		virtual void OnInt(const std::string& name, int& value) = 0;
		virtual void OnFloat(const std::string& name, float& value) = 0;
		virtual void OnBool(const std::string& name, bool& value) = 0;
		virtual void OnString(const std::string& name, std::string& value) = 0;
		virtual void OnFloat2(const std::string& name, DirectX::XMFLOAT2& value) = 0;
		virtual void OnFloat3(const std::string& name, DirectX::XMFLOAT3& value) = 0;
		virtual void OnFloat4(const std::string& name, DirectX::XMFLOAT4& value) = 0;
	};

	/// <summary>
	/// 1フィールド分の走査情報
	/// </summary>
	struct ComponentFieldInfo
	{
		std::string Name;
		std::function<void(void* inst, IComponentFieldVisitor& visitor)> Accept;
	};

	/// <summary>
	/// 型ごとに特殊化するディスクリプター
	/// </summary>
	template<typename T>
	struct ComponentTypeDescriptor
	{
		static const std::vector<ComponentFieldInfo>& Fields();
	};

	/// <summary>
	/// ビジターを適用する
	/// </summary>
	template<typename T>
	void VisitComponentFields(T& obj, IComponentFieldVisitor& visitor)
	{
		for (const auto& field : ComponentTypeDescriptor<T>::Fields())
			field.Accept(&obj, visitor);
	}
}

#define ECS_REFLECT_BEGIN(Type)                                                             \
	namespace ecs {                                                                          \
	template<> inline const std::vector<ecs::ComponentFieldInfo>&                            \
		ecs::ComponentTypeDescriptor<Type>::Fields()                                        \
	{                                                                                        \
		using _T = Type;                                                                     \
		static const std::vector<ecs::ComponentFieldInfo> sFields = {                        \

#define ECS_REFLECT_FIELD(FieldName)                                                        \
			ecs::ComponentFieldInfo {                                                        \
				#FieldName,                                                                  \
				[](void* inst, ecs::IComponentFieldVisitor& v)                              \
				{                                                                            \
					auto& self = *static_cast<_T*>(inst);                                   \
					using F = std::decay_t<decltype(self.FieldName)>;                       \
					if constexpr (std::is_same_v<F, int>)                     v.OnInt(#FieldName, self.FieldName); \
					else if constexpr (std::is_same_v<F, float>)              v.OnFloat(#FieldName, self.FieldName); \
					else if constexpr (std::is_same_v<F, bool>)               v.OnBool(#FieldName, self.FieldName); \
					else if constexpr (std::is_same_v<F, std::string>)        v.OnString(#FieldName, self.FieldName); \
					else if constexpr (std::is_same_v<F, DirectX::XMFLOAT2>)  v.OnFloat2(#FieldName, self.FieldName); \
					else if constexpr (std::is_same_v<F, DirectX::XMFLOAT3>)  v.OnFloat3(#FieldName, self.FieldName); \
					else if constexpr (std::is_same_v<F, DirectX::XMFLOAT4>)  v.OnFloat4(#FieldName, self.FieldName); \
				}                                                                            \
			},

#define ECS_REFLECT_FIELD_ENUM(FieldName)                                                   \
			ecs::ComponentFieldInfo {                                                        \
				#FieldName,                                                                  \
				[](void* inst, ecs::IComponentFieldVisitor& v)                              \
				{                                                                            \
					auto& self = *static_cast<_T*>(inst);                                   \
					static_assert(sizeof(self.FieldName) == sizeof(int),                    \
						"ECS_REFLECT_FIELD_ENUM requires an int-sized enum");               \
					v.OnInt(#FieldName, reinterpret_cast<int&>(self.FieldName));            \
				}                                                                            \
			},

#define ECS_REFLECT_FIELD_ACCESSOR(Name, Type, Getter, Setter)                              \
			ecs::ComponentFieldInfo {                                                        \
				#Name,                                                                       \
				[](void* inst, ecs::IComponentFieldVisitor& v)                              \
				{                                                                            \
					auto& self = *static_cast<_T*>(inst);                                   \
					Type temp = self.Getter();                                              \
					if constexpr (std::is_same_v<Type, DirectX::XMFLOAT2>) v.OnFloat2(#Name, temp); \
					else if constexpr (std::is_same_v<Type, DirectX::XMFLOAT3>) v.OnFloat3(#Name, temp); \
					else if constexpr (std::is_same_v<Type, DirectX::XMFLOAT4>) v.OnFloat4(#Name, temp); \
					else if constexpr (std::is_same_v<Type, float>) v.OnFloat(#Name, temp);  \
					self.Setter(temp);                                                       \
				}                                                                            \
			},

#define ECS_REFLECT_END()                                                                   \
		};                                                                                   \
		return sFields;                                                                      \
	}                                                                                        \
	}
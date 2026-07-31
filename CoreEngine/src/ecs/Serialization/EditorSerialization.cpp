#include "pch.h"
#include "EditorSerialization.h"

#include "PlaceableComponentReflection.h"

#include <ecs/entity/EntityTag.h>
#include <ecs/component/Common/NameComponent.h>
#include <ecs/component/Common/AssetKeyComponent.h>

#include <graphics/Fbx/Resource/FbxResourceManager.h>
#include <graphics/PrimitiveModel/Resource/PrimitiveResourceManager.h>
#include <graphics/Texture/TextureManager.h>
#include <graphics/Texture/Texture.h>

#include <json/json.hpp>
#include <fstream>
#include <vector>

namespace ecs
{
	namespace
	{
		// ── 構造体 → JSON ────────────────────────────────────────
		class JsonComponentSerializeVisitor final : public IComponentFieldVisitor
		{
		public:
			explicit JsonComponentSerializeVisitor(nlohmann::json& j) : mJson(j) {}

			void OnInt(const std::string& name, int& v) override { mJson[name] = v; }
			void OnFloat(const std::string& name, float& v) override { mJson[name] = v; }
			void OnBool(const std::string& name, bool& v) override { mJson[name] = v; }
			void OnString(const std::string& name, std::string& v) override { mJson[name] = v; }

			void OnFloat2(const std::string& name, DirectX::XMFLOAT2& v) override
			{
				mJson[name] = { v.x, v.y };
			}
			void OnFloat3(const std::string& name, DirectX::XMFLOAT3& v) override
			{
				mJson[name] = { v.x, v.y, v.z };
			}
			void OnFloat4(const std::string& name, DirectX::XMFLOAT4& v) override
			{
				mJson[name] = { v.x, v.y, v.z, v.w };
			}

		private:
			nlohmann::json& mJson;
		};

		// ── JSON → 構造体 (キーが無ければスキップ = 部分更新・後方互換対応) ──
		class JsonComponentDeserializeVisitor final : public IComponentFieldVisitor
		{
		public:
			explicit JsonComponentDeserializeVisitor(const nlohmann::json& j) : mJson(j) {}

			void OnInt(const std::string& name, int& v) override
			{
				if (mJson.contains(name) && mJson[name].is_number_integer()) v = mJson[name].get<int>();
			}
			void OnFloat(const std::string& name, float& v) override
			{
				if (mJson.contains(name) && mJson[name].is_number()) v = mJson[name].get<float>();
			}
			void OnBool(const std::string& name, bool& v) override
			{
				if (mJson.contains(name) && mJson[name].is_boolean()) v = mJson[name].get<bool>();
			}
			void OnString(const std::string& name, std::string& v) override
			{
				if (mJson.contains(name) && mJson[name].is_string()) v = mJson[name].get<std::string>();
			}
			void OnFloat2(const std::string& name, DirectX::XMFLOAT2& v) override
			{
				if (!mJson.contains(name) || !mJson[name].is_array() || mJson[name].size() != 2) return;
				v = { mJson[name][0].get<float>(), mJson[name][1].get<float>() };
			}
			void OnFloat3(const std::string& name, DirectX::XMFLOAT3& v) override
			{
				if (!mJson.contains(name) || !mJson[name].is_array() || mJson[name].size() != 3) return;
				v = { mJson[name][0].get<float>(), mJson[name][1].get<float>(), mJson[name][2].get<float>() };
			}
			void OnFloat4(const std::string& name, DirectX::XMFLOAT4& v) override
			{
				if (!mJson.contains(name) || !mJson[name].is_array() || mJson[name].size() != 4) return;
				v = { mJson[name][0].get<float>(), mJson[name][1].get<float>(),
					  mJson[name][2].get<float>(), mJson[name][3].get<float>() };
			}

		private:
			const nlohmann::json& mJson;
		};

		template<typename T>
		nlohmann::json SerializeComponent(T& component)
		{
			nlohmann::json j;
			JsonComponentSerializeVisitor visitor(j);
			VisitComponentFields(component, visitor);
			return j;
		}

		template<typename T>
		void DeserializeComponent(const nlohmann::json& j, T& component)
		{
			JsonComponentDeserializeVisitor visitor(j);
			VisitComponentFields(component, visitor);
		}

		/// <summary>
		/// AssetKey から FbxComponent::Resource を解決する。
		/// PrimitiveResourceManager のキャッシュを優先し、無ければ
		/// 実ファイルパスとみなして FbxResourceManager::Load を試みる。
		/// </summary>
		graphics::FbxResource* ResolveFbxResource(const std::string& assetKey)
		{
			if (graphics::FbxResource* prim = graphics::PrimitiveResourceManager::Get().GetResource(assetKey))
			{
				return prim;
			}
			return graphics::FbxResourceManager::Get().Load(assetKey);
		}
	} // 無名namespace

	void EditorSerialization::ClearPlaceableEntities(entt::registry& registry)
	{
		auto view = registry.view<ecs::PlaceableTag>();
		registry.destroy(view.begin(), view.end());
	}

	std::string EditorSerialization::SerializeToString(entt::registry& registry)
	{
		nlohmann::json root = nlohmann::json::array();

		registry.view<ecs::PlaceableTag, ecs::Transform>().each(
			[&](entt::entity entity, ecs::Transform& transform)
			{
				nlohmann::json entry;

				if (auto* name = registry.try_get<ecs::NameComponent>(entity))
					entry["Name"] = name->Name;

				entry["Transform"] = SerializeComponent(transform);

				if (auto* assetKey = registry.try_get<ecs::AssetKeyComponent>(entity))
					entry["AssetKey"] = assetKey->Key;

				if (auto* fbx = registry.try_get<ecs::FbxComponent>(entity))
					entry["Fbx"] = SerializeComponent(*fbx);

				if (auto* sprite = registry.try_get<ecs::Sprite>(entity))
					entry["Sprite"] = SerializeComponent(*sprite);

				if (auto* light = registry.try_get<ecs::DirectionalLightComponent>(entity))
					entry["DirectionalLight"] = SerializeComponent(*light);

				if (auto* light = registry.try_get<ecs::PointLightComponent>(entity))
					entry["PointLight"] = SerializeComponent(*light);

				if (auto* light = registry.try_get<ecs::SpotLightComponent>(entity))
					entry["SpotLight"] = SerializeComponent(*light);

				if (auto* collider = registry.try_get<ecs::ColliderComponent>(entity))
					entry["Collider"] = SerializeComponent(*collider);

				if (auto* rigidBody = registry.try_get<ecs::RigidBodyComponent>(entity))
					entry["RigidBody"] = SerializeComponent(*rigidBody);

				root.push_back(std::move(entry));
			});

		return root.dump(4);
	}

	void EditorSerialization::DeserializeFromString(entt::registry& registry, const std::string& json)
	{
		nlohmann::json root;
		try
		{
			root = nlohmann::json::parse(json);
		}
		catch (const nlohmann::json::exception&)
		{
			return; // パース失敗時は registry に触れず終了(既存状態を保持)
		}
		if (!root.is_array()) return;

		// パース成功が確認できてから既存の Placeable エンティティを消す
		ClearPlaceableEntities(registry);

		for (const auto& entry : root)
		{
			entt::entity entity = registry.create();
			registry.emplace<ecs::PlaceableTag>(entity);

			if (entry.contains("Name") && entry["Name"].is_string())
				registry.emplace<ecs::NameComponent>(entity, entry["Name"].get<std::string>());

			auto& transform = registry.emplace<ecs::Transform>(entity);
			if (entry.contains("Transform"))
				DeserializeComponent(entry["Transform"], transform);

			std::string assetKey;
			if (entry.contains("AssetKey") && entry["AssetKey"].is_string())
				assetKey = entry["AssetKey"].get<std::string>();

			if (entry.contains("Fbx"))
			{
				auto& fbx = registry.emplace<ecs::FbxComponent>(entity);
				DeserializeComponent(entry["Fbx"], fbx);
				if (!assetKey.empty())
				{
					registry.emplace<ecs::AssetKeyComponent>(entity, assetKey);
					fbx.Resource = ResolveFbxResource(assetKey);
				}
			}

			if (entry.contains("Sprite") && !assetKey.empty())
			{
				if (graphics::Texture* texture = graphics::TextureManager::Get().GetOrLoad(assetKey))
				{
					auto& sprite = registry.emplace<ecs::Sprite>(entity, texture);
					DeserializeComponent(entry["Sprite"], sprite);
					registry.emplace<ecs::AssetKeyComponent>(entity, assetKey);
				}
			}

			if (entry.contains("DirectionalLight"))
			{
				auto& light = registry.emplace<ecs::DirectionalLightComponent>(entity);
				DeserializeComponent(entry["DirectionalLight"], light);
			}
			if (entry.contains("PointLight"))
			{
				auto& light = registry.emplace<ecs::PointLightComponent>(entity);
				DeserializeComponent(entry["PointLight"], light);
			}
			if (entry.contains("SpotLight"))
			{
				auto& light = registry.emplace<ecs::SpotLightComponent>(entity);
				DeserializeComponent(entry["SpotLight"], light);
			}
			if (entry.contains("Collider"))
			{
				auto& collider = registry.emplace<ecs::ColliderComponent>(entity);
				DeserializeComponent(entry["Collider"], collider);
			}
			if (entry.contains("RigidBody"))
			{
				auto& rigidBody = registry.emplace<ecs::RigidBodyComponent>(entity);
				DeserializeComponent(entry["RigidBody"], rigidBody);

				// Jolt ハンドルは必ず未生成状態に戻す。
				// PhysicsSystem::BuildPendingBodies が次フレームで Body を再生成する。
				rigidBody.BodyID = JPH::BodyID();
				rigidBody.IsBodyCreated = false;
			}
		}
	}

	bool EditorSerialization::SaveToFile(entt::registry& registry, const std::string& filePath)
	{
		std::ofstream f(filePath);
		if (!f.is_open()) return false;
		f << SerializeToString(registry);
		return true;
	}

	bool EditorSerialization::LoadFromFile(entt::registry& registry, const std::string& filePath)
	{
		std::ifstream f(filePath);
		if (!f.is_open()) return false;

		std::string content(
			(std::istreambuf_iterator<char>(f)),
			std::istreambuf_iterator<char>());

		DeserializeFromString(registry, content);
		return true;
	}
}

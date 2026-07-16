#include "pch.h"
#include "EditorSystem.h"

#include<system/Input/InputManager.h>
#include<system/Camera/CameraSystem.h>
#include<system/Physics/System/PhysicsSystem.h>

#include<ecs/entity/EntityManager.h>
#include<ecs/entity/EntityTag.h>
#include<ecs/component/Common/NameComponent.h>
#include<ecs/component/Common/AssetKeyComponent.h>
#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/Fbx/FbxComponent.h>
#include<ecs/component/Light/LightComponent.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>

#include<graphics/PrimitiveModel/Resource/PrimitiveResourceManager.h>

using namespace DirectX;

namespace sys
{
	void EditorSystem::ArmPlacement(const std::string& placementKey)
	{
		mPendingPlacementKey = placementKey;
	}

	void EditorSystem::Update(entt::registry& registry)
	{
		auto& input = sys::InputManager::Get();
		const bool selectPressed = input.IsActionPressed("Select");
		const bool selectHeld = input.IsActionHeld("Select");

		// 配置待機中はクリックを配置に消費し、選択/ドラッグは行わない
		if (UpdatePlacement(registry, selectPressed))
		{
			return;
		}

		UpdateSelection(registry, selectPressed);

		if (input.IsActionPressed("Delete"))
		{
			DeleteSelected(registry);
		}

		UpdateDrag(registry, selectHeld);
	}

	void EditorSystem::DeleteSelected(entt::registry& registry)
	{
		auto selectedView = registry.view<ecs::SelectedTag, ecs::PlaceableTag>();
		if (selectedView.begin() == selectedView.end())
		{
			return;
		}

		const entt::entity selected = *selectedView.begin();
		registry.destroy(selected);
		mIsDragging = false;
	}

	bool EditorSystem::UpdatePlacement(entt::registry& registry, bool selectPressed)
	{
		if (mPendingPlacementKey.empty())
		{
			return false;
		}

		if (selectPressed)
		{
			auto& cameraSys = sys::CameraSystem::Get();
			if (cameraSys.HasMainCamera())
			{
				const XMFLOAT2 mousePos = sys::InputManager::Get().GetMouseVirtualPosition();
				XMFLOAT3 groundPos;
				if (cameraSys.ScreenPointToWorldOnPlaneY(registry, mousePos, 0.0f, groundPos))
				{
					const entt::entity spawned = SpawnPlacedObject(registry, mPendingPlacementKey, groundPos);
					if (registry.valid(spawned))
					{
						registry.clear<ecs::SelectedTag>();
						registry.emplace<ecs::SelectedTag>(spawned);
					}
				}
			}

			mPendingPlacementKey.clear();
		}

		return true;
	}

	void EditorSystem::UpdateSelection(entt::registry& registry, bool selectPressed)
	{
		if (!selectPressed)
		{
			return;
		}

		auto& cameraSys = sys::CameraSystem::Get();
		if (!cameraSys.HasMainCamera())
		{
			return;
		}

		const XMFLOAT2 mousePos = sys::InputManager::Get().GetMouseVirtualPosition();
		const sys::Ray ray = cameraSys.ScreenPointToRay(registry, mousePos);

		entt::entity hitEntity = entt::null;
		XMFLOAT3 hitPoint = {};
		const bool hit = sys::PhysicsSystem::TryPickEntity(
			registry, ray.Origin, ray.Direction, 1000.0f, hitEntity, hitPoint);

		// 配置済みオブジェクト(PlaceableTag)以外はエディタ選択の対象にしない
		registry.clear<ecs::SelectedTag>();
		if (hit && registry.all_of<ecs::PlaceableTag>(hitEntity))
		{
			registry.emplace<ecs::SelectedTag>(hitEntity);
		}
	}

	void EditorSystem::UpdateDrag(entt::registry& registry, bool selectHeld)
	{
		auto selectedView = registry.view<ecs::SelectedTag, ecs::Transform>();
		if (selectedView.begin() == selectedView.end() || !selectHeld)
		{
			mIsDragging = false;
			return;
		}

		auto& cameraSys = sys::CameraSystem::Get();
		if (!cameraSys.HasMainCamera())
		{
			return;
		}

		const entt::entity selected = *selectedView.begin();
		auto& transform = registry.get<ecs::Transform>(selected);

		if (!mIsDragging)
		{
			mIsDragging = true;
			mDragPlaneY = transform.GetPosition().y;
		}

		const XMFLOAT2 mousePos = sys::InputManager::Get().GetMouseVirtualPosition();
		XMFLOAT3 worldPos;
		if (cameraSys.ScreenPointToWorldOnPlaneY(registry, mousePos, mDragPlaneY, worldPos))
		{
			transform.SetPosition(worldPos.x, mDragPlaneY, worldPos.z);
		}
	}

	entt::entity EditorSystem::SpawnPlacedObject(
		entt::registry& registry,
		const std::string& key,
		const XMFLOAT3& groundHitPos)
	{
		const entt::entity entity = ecs::EntityManager::Get().CreateEntity();
		registry.emplace<ecs::PlaceableTag>(entity);
		registry.emplace<ecs::NameComponent>(
			entity, key + "_" + std::to_string(static_cast<uint32_t>(entity)));

		// ライトなど物理を持たない配置物
		if (key == "PointLight")
		{
			auto& transform = registry.emplace<ecs::Transform>(entity);
			transform.SetPosition(groundHitPos.x, groundHitPos.y + 2.0f, groundHitPos.z);
			registry.emplace<ecs::PointLightComponent>(entity);
			return entity;
		}

		// プリミティブ(Box/Sphere 等)配置。Collider + RigidBody を付けて
		// クリック選択(Joltレイキャスト)と Play 時の物理挙動を有効にする。
		graphics::FbxResource* resource = graphics::PrimitiveResourceManager::Get().GetResource(key);
		if (resource == nullptr)
		{
			registry.destroy(entity);
			return entt::null;
		}

		// 底面が地面に接するよう、半分埋まらない高さに補正する
		auto& transform = registry.emplace<ecs::Transform>(entity);
		transform.SetPosition(groundHitPos.x, groundHitPos.y + 0.5f, groundHitPos.z);

		auto& fbx = registry.emplace<ecs::FbxComponent>(entity);
		fbx.Resource = resource;
		registry.emplace<ecs::AssetKeyComponent>(entity, key);

		if (key == "Sphere")
		{
			registry.emplace<ecs::ColliderComponent>(entity, ecs::ColliderComponent::MakeSphere(0.5f));
		}
		else
		{
			registry.emplace<ecs::ColliderComponent>(
				entity, ecs::ColliderComponent::MakeBox({ 0.5f, 0.5f, 0.5f }));
		}
		registry.emplace<ecs::RigidBodyComponent>(entity, ecs::RigidBodyComponent::MakeDynamic());

		return entity;
	}
}

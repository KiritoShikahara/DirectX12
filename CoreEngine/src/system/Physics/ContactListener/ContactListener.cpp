#include "pch.h"
#include "ContactListener.h"

#include<Jolt/Physics/Body/Body.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>

namespace sys
{
	ContactListener::ContactListener(entt::registry& registry)
		: mRegistry(registry)
	{
	}

	/// <summary>
	/// Body の UserData を entt::entity に変換する
	/// </summary>
	inline entt::entity sys::ContactListener::ToEntity(const JPH::Body& body)
	{
		return static_cast<entt::entity>(
			static_cast<uint32_t>(body.GetUserData()));
	}

	/// <summary>
	/// entity に CollisionEnterEvent がなければ生成し、OtherEntities に other を追加する
	/// </summary>
	void ContactListener::AppendCollisionEnter(entt::entity entity, entt::entity other)
	{
		if (!mRegistry.valid(entity)) return;

		auto* ev = mRegistry.try_get<ecs::CollisionEnterEvent>(entity);
		if (ev == nullptr)
		{
			ev = &mRegistry.emplace<ecs::CollisionEnterEvent>(entity);
		}
		ev->OtherEntities.push_back(other);
	}

	/// <summary>
	/// entity に SensorEnterEvent がなければ生成し、Visitors に visitor を追加する
	/// </summary>
	void ContactListener::AppendSensorEnter(entt::entity entity, entt::entity visitor)
	{
		if (!mRegistry.valid(entity)) return;

		auto* ev = mRegistry.try_get<ecs::SensorEnterEvent>(entity);
		if (ev == nullptr)
		{
			ev = &mRegistry.emplace<ecs::SensorEnterEvent>(entity);
		}
		ev->Visitors.push_back(visitor);
	}

	void ContactListener::OnContactAdded(
		const JPH::Body& inBody1,
		const JPH::Body& inBody2,
		const JPH::ContactManifold& /*inManifold*/,
		JPH::ContactSettings&      /*ioSettings*/)
	{
		const entt::entity entityA = ToEntity(inBody1);
		const entt::entity entityB = ToEntity(inBody2);

		const bool isSensorA = inBody1.IsSensor();
		const bool isSensorB = inBody2.IsSensor();

		if (isSensorA)
		{
			// A がセンサー → A に SensorEnterEvent、B が侵入者
			AppendSensorEnter(entityA, entityB);
		}
		else if (isSensorB)
		{
			// B がセンサー → B に SensorEnterEvent、A が侵入者
			AppendSensorEnter(entityB, entityA);
		}
		else
		{
			// 通常の物理衝突 → 両方に CollisionEnterEvent
			AppendCollisionEnter(entityA, entityB);
			AppendCollisionEnter(entityB, entityA);
		}
	}
}


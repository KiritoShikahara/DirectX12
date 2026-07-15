#include "pch.h"
#include "ContactListener.h"

#include<Jolt/Physics/Body/Body.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>

namespace sys
{
	namespace
	{
		// 1フレームで発生しうる保留イベント数の目安。
		// 毎フレームの再確保を避けるため初期予約しておく。
		constexpr size_t kPendingEventsReserve = 256;
	}

	ContactListener::ContactListener(entt::registry& registry)
		: mRegistry(registry)
	{
		mPendingEvents.reserve(kPendingEventsReserve);
	}

	/// <summary>
	/// Body の UserData から entt::entity へ変換する
	/// </summary>
	inline entt::entity sys::ContactListener::ToEntity(const JPH::Body& body)
	{
		return static_cast<entt::entity>(
			static_cast<uint32_t>(body.GetUserData()));
	}

	void ContactListener::PushPendingEvent(EventKind kind, entt::entity entity, entt::entity other)
	{
		std::lock_guard<std::mutex> lock(mPendingMutex);
		mPendingEvents.push_back({ kind, entity, other });
	}

	void ContactListener::OnContactAdded(
		const JPH::Body& inBody1,
		const JPH::Body& inBody2,
		const JPH::ContactManifold& /*inManifold*/,
		JPH::ContactSettings&      /*ioSettings*/)
	{
		// この関数は Jolt の衝突検出ジョブスレッドから並行に呼ばれうるため、
		// entt::registry には一切触れず、保留イベントバッファへ積むだけにする。
		const entt::entity entityA = ToEntity(inBody1);
		const entt::entity entityB = ToEntity(inBody2);

		const bool isSensorA = inBody1.IsSensor();
		const bool isSensorB = inBody2.IsSensor();

		if (isSensorA)
		{
			// A がセンサー → A に SensorEnterEvent、B が侵入者
			PushPendingEvent(EventKind::SensorEnter, entityA, entityB);
		}
		else if (isSensorB)
		{
			// B がセンサー → B に SensorEnterEvent、A が侵入者
			PushPendingEvent(EventKind::SensorEnter, entityB, entityA);
		}
		else
		{
			// 通常の物理衝突 → 双方に CollisionEnterEvent
			PushPendingEvent(EventKind::CollisionEnter, entityA, entityB);
			PushPendingEvent(EventKind::CollisionEnter, entityB, entityA);
		}
	}

	void ContactListener::FlushPendingEvents(entt::registry& registry)
	{
		// メインスレッドのみから呼ばれる想定。ロックは Jolt 側との整合性のため。
		std::lock_guard<std::mutex> lock(mPendingMutex);

		for (const PendingEvent& ev : mPendingEvents)
		{
			if (!registry.valid(ev.Entity)) continue;

			if (ev.Kind == EventKind::CollisionEnter)
			{
				auto* comp = registry.try_get<ecs::CollisionEnterEvent>(ev.Entity);
				if (comp == nullptr)
				{
					comp = &registry.emplace<ecs::CollisionEnterEvent>(ev.Entity);
				}
				comp->OtherEntities.push_back(ev.Other);
			}
			else // SensorEnter
			{
				auto* comp = registry.try_get<ecs::SensorEnterEvent>(ev.Entity);
				if (comp == nullptr)
				{
					comp = &registry.emplace<ecs::SensorEnterEvent>(ev.Entity);
				}
				comp->Visitors.push_back(ev.Other);
			}
		}

		// capacity は保持したままクリアし、毎フレームの再確保を避ける
		mPendingEvents.clear();
	}
}

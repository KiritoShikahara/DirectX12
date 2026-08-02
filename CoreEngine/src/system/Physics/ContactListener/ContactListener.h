#pragma once

#include <entt/entt.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Utility/Export/Export.h>

#include <vector>
#include <mutex>
#include <cstdint>

namespace sys
{
	class ENGINE_API ContactListener final : public JPH::ContactListener
	{
	public:
		explicit ContactListener(entt::registry& registry);
		virtual ~ContactListener() = default;

		/// <summary>
		/// 衝突を受け入れるかどうかを返す
		/// </summary>
		JPH::ValidateResult OnContactValidate(
			const JPH::Body& inBody1,
			const JPH::Body& inBody2,
			JPH::RVec3Arg               inBaseOffset,
			const JPH::CollideShapeResult& inResult) override
		{
			return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		/// <summary>
		/// 衝突開始コールバック。
		/// Jolt の衝突検出ジョブスレッドから並行に呼ばれる可能性があるため、
		/// ここでは entt::registry に一切触れず、保留イベントバッファへ積むだけにする。
		/// </summary>
		void OnContactAdded(
			const JPH::Body& inBody1,
			const JPH::Body& inBody2,
			const JPH::ContactManifold& inManifold,
			JPH::ContactSettings& ioSettings) override;

		/// <summary>
		/// 衝突継続コールバック。接触が続いている間、毎フレーム呼ばれる。
		/// OnContactAdded と同様、CollisionStayEvent/SensorStayEvent を保留バッファへ積むだけにする。
		/// </summary>
		void OnContactPersisted(
			const JPH::Body& inBody1,
			const JPH::Body& inBody2,
			const JPH::ContactManifold& inManifold,
			JPH::ContactSettings& ioSettings) override;

		void OnContactRemoved(
			const JPH::SubShapeIDPair&) override {
		}

		/// <summary>
		/// OnContactAdded で蓄積された保留イベントを entt::registry へ反映する。
		/// </summary>
		void FlushPendingEvents(entt::registry& registry);

	private:
		/// <summary>
		/// Body の UserData から entt::entity へ変換する
		/// </summary>
		static entt::entity ToEntity(const JPH::Body& body);

		enum class EventKind : uint8_t
		{
			CollisionEnter,
			CollisionStay,
			SensorEnter,
			SensorStay,
		};

		/// <summary>
		/// OnContactAdded から積まれる保留イベント1件分。
		/// registry への反映は FlushPendingEvents でメインスレッドから行う。
		/// </summary>
		struct PendingEvent
		{
			EventKind    Kind;
			entt::entity Entity;
			entt::entity Other;
		};

		/// <summary>entity に対応する保留イベントをバッファへ積む（スレッドセーフ）</summary>
		void PushPendingEvent(EventKind kind, entt::entity entity, entt::entity other);

	private:
		entt::registry& mRegistry;

		/// <summary>mPendingEvents を保護する排他制御。Jolt のジョブスレッドから並行に書き込まれる。</summary>
		std::mutex mPendingMutex;

		/// <summary>OnContactAdded から積まれた未反映のイベント。FlushPendingEvents でクリアされる。</summary>
		std::vector<PendingEvent> mPendingEvents;
	};
}

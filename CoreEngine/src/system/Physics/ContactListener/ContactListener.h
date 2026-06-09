#pragma once

#include <entt/entt.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Utility/Export/Export.h>

namespace sys
{
	class ContactListener final : public JPH::ContactListener
	{
	public:
		explicit ContactListener(entt::registry& registry);
		virtual ~ContactListener() = default;

        /// <summary>
        /// 衝突を受け入れるかどうかを返す（常に全受け入れ）
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
        /// Body の UserData から entt::entity を取り出し、
        /// CollisionEnterEvent または SensorEnterEvent に追記する。
        /// </summary>
        void OnContactAdded(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings) override;

        // 継続・終了は今は使わない
        void OnContactPersisted(
            const JPH::Body&,
            const JPH::Body&,
            const JPH::ContactManifold&,
            JPH::ContactSettings&) override {
        }

        void OnContactRemoved(
            const JPH::SubShapeIDPair&) override {
        }

	private:
        /// <summary>
        /// Body の UserData を entt::entity に変換する
        /// </summary>
        static entt::entity ToEntity(const JPH::Body& body);

        /// <summary>
        /// entity に CollisionEnterEvent がなければ生成し、OtherEntities に other を追加する
        /// </summary>
        void AppendCollisionEnter(entt::entity entity, entt::entity other);

        /// <summary>
        /// entity に SensorEnterEvent がなければ生成し、Visitors に visitor を追加する
        /// </summary>
        void AppendSensorEnter(entt::entity entity, entt::entity visitor);

    private:
		entt::registry& mRegistry;
	};
}



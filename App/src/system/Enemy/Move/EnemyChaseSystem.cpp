#include "apppch.h"
#include "EnemyChaseSystem.h"

#include<system/MoveDirection/MoveDirectionComponent.h>
#include<Tag/EntityTag.h>
#include"EnemyChaseComponent.h"

using namespace DirectX;

namespace ecs
{
	void EnemyChaseSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto playerView = registry.view<PlayerTag, Transform>();
		if (playerView.size_hint() == 0)
		{
			// プレイヤー不在なら何もしない
			return;
		}

		const entt::entity playerEntity = *playerView.begin();
		const XMFLOAT3 playerPos = registry.get<Transform>(playerEntity).GetPosition();
		const XMVECTOR vPlayer = XMLoadFloat3(&playerPos);

		constexpr float kEpsilon = 1e-4f;

		// プレイヤーに向けて移動

        registry.view<EnemyTag, EnemyChaseComponent, Transform, RigidBodyComponent, MoveDirectionComponent>().each(
            [&](EnemyChaseComponent& chase,
                Transform& transform,
                RigidBodyComponent& rigidBody,
                MoveDirectionComponent& moveDir)
            {
                const XMFLOAT3 pos = transform.GetPosition();

                // プレイヤーへのベクトル（水平面のみ：Y を無視）
                XMVECTOR toPlayer = XMVectorSubtract(vPlayer, XMLoadFloat3(&pos));
                toPlayer = XMVectorSetY(toPlayer, 0.0f);

                const float dist = XMVectorGetX(XMVector3Length(toPlayer));

                // ほぼ同一座標なら停止（正規化不能）
                if (dist <= kEpsilon)
                {
                    moveDir.IsMoving = false;
                    return;
                }

                const XMVECTOR dir = XMVector3Normalize(toPlayer);

                // 向きは間合い内でも更新し続ける（プレイヤーを向く）
                XMStoreFloat3(&moveDir.Direction, dir);

                // 停止間合いより遠いときだけ移動速度を積む
                if (dist > chase.StopDistance)
                {
                    XMFLOAT3 velocity;
                    XMStoreFloat3(&velocity, XMVectorScale(dir, chase.MoveSpeed));

                    rigidBody.MoveVelocity = velocity;
                    rigidBody.HasMoveRequest = true;
                    moveDir.IsMoving = true;
                }
                else
                {
                    // 間合い内：移動リクエストは積まない（MoveVelocity はフレーム末にクリアされる）
                    moveDir.IsMoving = false;
                }
            });
	}
}
	 

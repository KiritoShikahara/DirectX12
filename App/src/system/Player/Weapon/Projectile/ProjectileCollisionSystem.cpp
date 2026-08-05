#include "apppch.h"
#include "ProjectileCollisionSystem.h"

#include"ProjectileComponent.h"

#include<system/Physics/System/PhysicsSystem.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<graphics/Line/Renderer/PhysicsDebugRenderer.h>
#include<Tag/EntityTag.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Effect/TemporaryLifetimeComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Player/PlayerActionLock.h>
#include<Scene/Game/Debug/GameDebugSettings.h>
#include<ecs/component/Fbx/FbxComponent.h>

#include<algorithm>
#include<random>

namespace
{
	constexpr float kEffectReferenceRadius = 2.0f;
	constexpr float kDebugWireLifetime = 0.3f;

	std::mt19937& GetRandomEngine()
	{
		// プロセス全体で1つの乱数エンジンを使い回す、毎フレーム再生成しない
		static std::mt19937 engine = ::debug::GameDebugSettings::Get().MakeRandomEngine();
		return engine;
	}
}

namespace ecs
{
	void ProjectileCollisionSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// 必殺技演出中は既存の弾の命中判定も一時停止させる
		if (ecs::IsPlayerUltimateActive(registry)) return;

		mHitProjectiles.clear();
		mHitResults.clear();
		mSplitRequests.clear();

		// SpawnExplosionEffectはエンティティ生成でTransformプールを書き換えEnTTのイテレータを不正化するため、view内では登録のみ行う
		registry.view<ecs::ProjectileComponent, ecs::SensorEnterEvent, ecs::Transform>().each(
			[&](entt::entity entity,
				ecs::ProjectileComponent& projectile,
				ecs::SensorEnterEvent& sensorEvent,
				ecs::Transform& transform)
			{
				// 敵タグを持つ侵入者のうち最初の1体を命中対象とみなす、増殖弾はこの敵を次の対象探索から除外する
				entt::entity hitEnemyEntity = entt::null;
				for (entt::entity other : sensorEvent.Visitors)
				{
					if (registry.valid(other) && registry.all_of<ecs::EnemyTag>(other))
					{
						hitEnemyEntity = other;
						break;
					}
				}

				if (!registry.valid(hitEnemyEntity)) return;

				// 着弾エフェクトのスケールはHitEffectVisualRadiusが指定されていればそちらを優先する。
				// 増殖弾の子弾コライダーサイズ(SplitRequest.VisualRadius)には影響させないため、
				// projectile.VisualRadius自体は書き換えずここでだけ差し替える
				const float hitEffectVisualRadius = (projectile.HitEffectVisualRadius >= 0.0f)
					? projectile.HitEffectVisualRadius : projectile.VisualRadius;

				mHitResults.push_back({
					transform.GetPosition(),
					projectile.ExplosionRadius,
					hitEffectVisualRadius,
					projectile.Damage,
					projectile.ExplosionEffectPath,
					projectile.ExplosionAtGroundLevel });

				// MaxGeneration/SplitCountが設定されまだ増殖できる世代であれば、破棄と同時に子弾を生成する予約をしておく
				if (projectile.SplitCount > 0 && projectile.Generation < projectile.MaxGeneration)
				{
					mSplitRequests.push_back({
						transform.GetPosition(),
						hitEnemyEntity,
						projectile.Generation + 1,
						projectile.SplitCount,
						projectile.MaxGeneration,
						projectile.SplitSearchRadius,
						projectile.Speed,
						projectile.Damage,
						projectile.ExplosionRadius,
						projectile.VisualRadius,
						projectile.ExplosionEffectPath,
						projectile.LifeTime,
						projectile.Owner,
						projectile.VisualMeshResource,
						projectile.VisualMeshScale,
						projectile.VisualMeshColor });
				}

				// PierceCountが残っている間は消滅させず貫通回数だけを消費する、0未満になった時点で破棄する
				if (projectile.PierceCount > 0)
				{
					--projectile.PierceCount;
				}
				else
				{
					mHitProjectiles.push_back(entity);
				}
			});

		// view 走査完了後にダメージ適用・エフェクト生成・弾の破棄・子弾の増殖を行う
		for (const HitResult& hit : mHitResults)
		{
			ApplyExplosionDamage(registry, hit.ImpactPos, hit.ExplosionRadius, hit.Damage);
			SpawnExplosionEffect(registry, hit.ImpactPos, hit.ExplosionEffectPath, hit.ExplosionRadius, hit.VisualRadius, hit.ExplosionAtGroundLevel);
		}

		for (const SplitRequest& request : mSplitRequests)
		{
			SpawnSplitProjectiles(registry, request);
		}

		for (entt::entity entity : mHitProjectiles)
		{
			registry.destroy(entity);
		}
	}

	void ProjectileCollisionSystem::ApplyExplosionDamage(
		entt::registry& registry,
		const DirectX::XMFLOAT3& center,
		float radius,
		float damage)
	{
		mOverlapped.clear();
		::sys::PhysicsSystem::OverlapSphere(registry, center, radius, mOverlapped);

		for (entt::entity entity : mOverlapped)
		{
			ecs::combatutil::ApplyDamageToEnemy(registry, entity, damage);
		}
	}

	void ProjectileCollisionSystem::SpawnExplosionEffect(
		entt::registry& registry,
		const DirectX::XMFLOAT3& position,
		const std::string& effectPath,
		float hitRadius,
		float visualRadius,
		bool explosionAtGroundLevel)
	{
		// 実際の判定半径hitRadiusを可視化する、HitRadiusMultiplierにより見た目より大きくなっているため優先して表示する
		if (graphics::PhysicsDebugRenderer::Get().IsEnabled())
		{
			auto& manager = ::ecs::EntityManager::Get();
			auto wireEntity = manager.CreateEntity();
			auto& transform = manager.AddComponent<ecs::Transform>(wireEntity);
			transform.SetPosition(position);
			auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(wireEntity);
			wire.Radius = hitRadius;
			wire.Color = { 1.0f, 0.4f, 0.1f, 1.0f }; // 炎らしいオレンジ
			manager.AddComponent<ecs::TemporaryLifetimeComponent>(wireEntity).RemainingTime = kDebugWireLifetime;
		}

		// 見た目のサイズは判定半径hitRadiusではなくvisualRadius基準で合わせる
		const float scale = visualRadius / kEffectReferenceRadius;

		// ExplosionAtGroundLevelの場合、着弾エフェクトのYだけ地面に固定する。ダメージ判定は実際の着弾位置のまま変えない
		const DirectX::XMFLOAT3 effectPosition = explosionAtGroundLevel
			? DirectX::XMFLOAT3{ position.x, 0.0f, position.z }
			: position;

		ecs::effectutil::PlayOneShotCombined(effectPath, effectPosition, scale);
	}

	void ProjectileCollisionSystem::SpawnSplitProjectiles(entt::registry& registry, const SplitRequest& request)
	{
		mSplitFound.clear();
		::sys::PhysicsSystem::OverlapSphere(registry, request.ImpactPos, request.SplitSearchRadius, mSplitFound);

		mSplitEnemies.clear();
		mSplitEnemies.reserve(mSplitFound.size());
		for (entt::entity entity : mSplitFound)
		{
			if (entity == request.ExcludedEnemy) continue;
			if (registry.all_of<ecs::EnemyTag>(entity) && registry.all_of<ecs::Transform>(entity))
			{
				mSplitEnemies.push_back(entity);
			}
		}
		if (mSplitEnemies.empty()) return;

		// 重複無しでランダムにSplitCount体まで選ぶ
		std::shuffle(mSplitEnemies.begin(), mSplitEnemies.end(), GetRandomEngine());
		const int count = std::min<int>(request.SplitCount, static_cast<int>(mSplitEnemies.size()));

		auto& manager = ::ecs::EntityManager::Get();

		for (int i = 0; i < count; ++i)
		{
			const auto& targetTransform = registry.get<ecs::Transform>(mSplitEnemies[i]);
			const DirectX::XMFLOAT3& targetPos = targetTransform.GetPosition();

			const float dx = targetPos.x - request.ImpactPos.x;
			const float dz = targetPos.z - request.ImpactPos.z;
			const float lenSq = dx * dx + dz * dz;
			DirectX::XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f };
			if (lenSq > 0.0001f)
			{
				const float invLen = 1.0f / std::sqrt(lenSq);
				direction = { dx * invLen, 0.0f, dz * invLen };
			}

			auto entity = manager.CreateEntity();

			auto& transform = manager.AddComponent<ecs::Transform>(entity);
			transform.SetPosition(request.ImpactPos);
			// VisualMeshScaleは既にTransform::SetScale用の倍率になっているためそのまま使う、コライダーは見た目半径VisualRadiusに合わせる
			transform.SetScale(request.VisualMeshScale);

			manager.AddComponent<ecs::ColliderComponent>(entity, ecs::ColliderComponent::MakeSphere(request.VisualRadius));
			registry.emplace<ecs::SensorTagComponent>(entity);

			auto& rigid = manager.AddComponent<ecs::RigidBodyComponent>(entity, ecs::RigidBodyComponent::MakeKinematic());
			rigid.GravityFactor = 0.0f;

			auto& projectile = manager.AddComponent<ecs::ProjectileComponent>(entity);
			projectile.Direction = direction;
			projectile.Speed = request.Speed;
			projectile.Damage = request.Damage;
			projectile.ExplosionRadius = request.ExplosionRadius;
			projectile.VisualRadius = request.VisualRadius;
			projectile.ExplosionEffectPath = request.ExplosionEffectPath;
			projectile.LifeTime = request.LifeTime;
			projectile.Owner = request.Owner;
			projectile.Generation = request.NextGeneration;
			projectile.MaxGeneration = request.MaxGeneration;
			projectile.SplitCount = request.SplitCount;
			projectile.SplitSearchRadius = request.SplitSearchRadius;
			projectile.VisualMeshResource = request.VisualMeshResource;
			projectile.VisualMeshScale = request.VisualMeshScale;
			projectile.VisualMeshColor = request.VisualMeshColor;

			if (request.VisualMeshResource != nullptr)
			{
				auto& fbx = manager.AddComponent<ecs::FbxComponent>(entity);
				fbx.Resource = request.VisualMeshResource;
				fbx.AutoPivot = false;
				fbx.CustomColor = request.VisualMeshColor;
			}
		}
	}
}

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
	// エフェクト素材は概ねこの半径感で作られている想定の暫定値。
	// 実際の判定半径とのズレ(見た目は小さいのに判定は大きい/その逆)を軽減するための概算スケール。
	constexpr float kEffectReferenceRadius = 2.0f;

	// 判定半径可視化用ワイヤーの表示時間(秒)。EffectComponentのautoDeleteに乗らない
	// デバッグ専用エンティティのため、TemporaryLifetimeComponentで明示的に破棄する。
	constexpr float kDebugWireLifetime = 0.3f;

	// プロセス全体で1つの乱数エンジンを使い回す（毎フレーム再生成しない。Meteor等と同じ方針）
	std::mt19937& GetRandomEngine()
	{
		static std::mt19937 engine = ::debug::GameDebugSettings::Get().MakeRandomEngine();
		return engine;
	}
}

namespace ecs
{
	void ProjectileCollisionSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// 必殺技演出中は既存の弾の命中判定も一時停止させる(Homing Missile/Bone Spear等の
		// 自動発動武器の弾も含むため、Flicker Strike中は止めない。PlayerActionLock.h参照)
		if (ecs::IsPlayerUltimateActive(registry)) return;

		mHitProjectiles.clear();
		mHitResults.clear();
		mSplitRequests.clear();

		// 命中情報はここでは記録するだけにする。
		// SpawnExplosionEffect() はエンティティ生成 + Transform コンポーネント追加を行うため、
		// この view が走査中の Transform プールをその場で書き換えることになり、
		// EnTT のイテレータを不正化する（走査中の残りの弾の判定が壊れる/クラッシュしうる）。
		// そのため view.each() 内では登録のみ行い、実際の生成・破棄は走査完了後にまとめて行う。
		registry.view<ecs::ProjectileComponent, ecs::SensorEnterEvent, ecs::Transform>().each(
			[&](entt::entity entity,
				ecs::ProjectileComponent& projectile,
				ecs::SensorEnterEvent& sensorEvent,
				ecs::Transform& transform)
			{
				// 敵タグを持つ侵入者のうち最初の1体を命中対象とみなす（当たり判定は敵とのみ）。
				// 増殖弾(Ricochet)の場合、この敵を次の対象探索から除外する基準にする
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

				mHitResults.push_back({
					transform.GetPosition(),
					projectile.ExplosionRadius,
					projectile.VisualRadius,
					projectile.Damage,
					projectile.ExplosionEffectPath,
					projectile.ExplosionAtGroundLevel });

				// MaxGeneration/SplitCount(増殖弾/Ricochet専用、通常弾は0)が設定されており、
				// まだ増殖できる世代であれば、破棄と同時に子弾を生成する予約をしておく
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

				// PierceCount(貫通弾/Bone Spear専用、通常弾は0)が残っている間は消滅させず、
				// 貫通回数だけを消費する。0未満になった時点で通常弾と同じく破棄する
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

	/// <summary>命中位置に爆発ダメージを適用する（敵タグ以外は無視する）</summary>
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

	/// <summary>着弾エフェクトを一度だけ再生する一時エンティティを生成する</summary>
	void ProjectileCollisionSystem::SpawnExplosionEffect(
		entt::registry& registry,
		const DirectX::XMFLOAT3& position,
		const std::string& effectPath,
		float hitRadius,
		float visualRadius,
		bool explosionAtGroundLevel)
	{
		// 実際の判定半径(hitRadius)を可視化する（ImGui「Physics Debug」→「Show Colliders」）。
		// HitRadiusMultiplierにより見た目(visualRadius)より大きくなっているため、
		// デバッグ表示は実際にダメージが及ぶ範囲(hitRadius)を優先して表示する。
		// トグルOFF中は描画されず無駄なため、ONの時だけ生成する。
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

		// 見た目のサイズは判定半径(hitRadius)ではなくvisualRadius基準で合わせる
		// （HitRadiusMultiplierで判定だけ拡大しても見た目は変えないため）。
		// effectPathは';'区切りで複数指定可能(ecs::effectutil::PlayOneShotCombined参照)。
		const float scale = visualRadius / kEffectReferenceRadius;

		// ExplosionAtGroundLevel(FireBolt等)の場合、着弾エフェクトのYだけ地面(0)に固定する。
		// 弾は胸の高さ(HeightOffset)を飛ぶが、爆発は地面で起きているように見せたいため。
		// ダメージ判定(ApplyExplosionDamage)やデバッグワイヤーは実際の着弾位置(position)のまま変えない。
		const DirectX::XMFLOAT3 effectPosition = explosionAtGroundLevel
			? DirectX::XMFLOAT3{ position.x, 0.0f, position.z }
			: position;

		ecs::effectutil::PlayOneShotCombined(effectPath, effectPosition, scale);
	}

	/// <summary>命中位置のSplitSearchRadius内から(命中した敵を除いて)ランダムに
	/// 最大SplitCount体の敵を選び、それぞれへ向かう子弾(Generation+1)を生成する</summary>
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

		// 重複無しでランダムにSplitCount体まで選ぶ(Meteor等と同じ方針)
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
			// VisualMeshScaleは既に(プリミティブの実メッシュ半径を考慮した)Transform::SetScale用の
			// 倍率になっているため、そのまま使う。コライダーは見た目半径(VisualRadius)に合わせる
			// (この2つは意味が異なるため混同しないこと。RicochetWeaponSystem::Fire参照)
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

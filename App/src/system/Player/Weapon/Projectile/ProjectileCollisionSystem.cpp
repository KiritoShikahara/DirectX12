#include "apppch.h"
#include "ProjectileCollisionSystem.h"

#include"ProjectileComponent.h"

#include<system/Physics/System/PhysicsSystem.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<Tag/EntityTag.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Effect/TemporaryLifetimeComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>

namespace
{
	// エフェクト素材は概ねこの半径感で作られている想定の暫定値。
	// 実際の判定半径とのズレ(見た目は小さいのに判定は大きい/その逆)を軽減するための概算スケール。
	constexpr float kEffectReferenceRadius = 2.0f;

	// 判定半径可視化用ワイヤーの表示時間(秒)。EffectComponentのautoDeleteに乗らない
	// デバッグ専用エンティティのため、TemporaryLifetimeComponentで明示的に破棄する。
	constexpr float kDebugWireLifetime = 0.3f;
}

namespace ecs
{
	void ProjectileCollisionSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// 必殺技演出中は既存の弾の命中判定も一時停止させる
		if (ecs::IsPlayerUltimateActive(registry)) return;

		std::vector<entt::entity> hitProjectiles;

		// 命中情報はここでは記録するだけにする。
		// SpawnExplosionEffect() はエンティティ生成 + Transform コンポーネント追加を行うため、
		// この view が走査中の Transform プールをその場で書き換えることになり、
		// EnTT のイテレータを不正化する（走査中の残りの弾の判定が壊れる/クラッシュしうる）。
		// そのため view.each() 内では登録のみ行い、実際の生成・破棄は走査完了後にまとめて行う。
		struct HitResult
		{
			DirectX::XMFLOAT3 ImpactPos;
			float             ExplosionRadius;
			float             VisualRadius;
			float             Damage;
			std::string       ExplosionEffectPath;
		};
		std::vector<HitResult> hitResults;

		registry.view<ecs::ProjectileComponent, ecs::SensorEnterEvent, ecs::Transform>().each(
			[&](entt::entity entity,
				ecs::ProjectileComponent& projectile,
				ecs::SensorEnterEvent& sensorEvent,
				ecs::Transform& transform)
			{
				// 敵タグを持つ侵入者が一人でもいれば命中とみなす（当たり判定は敵とのみ）
				const bool hitEnemy = std::any_of(
					sensorEvent.Visitors.begin(), sensorEvent.Visitors.end(),
					[&](entt::entity other)
					{
						return registry.valid(other) && registry.all_of<ecs::EnemyTag>(other);
					});

				if (!hitEnemy) return;

				hitResults.push_back({
					transform.GetPosition(),
					projectile.ExplosionRadius,
					projectile.VisualRadius,
					projectile.Damage,
					projectile.ExplosionEffectPath });

				hitProjectiles.push_back(entity);
			});

		// view 走査完了後にダメージ適用・エフェクト生成・弾の破棄を行う
		for (const HitResult& hit : hitResults)
		{
			ApplyExplosionDamage(registry, hit.ImpactPos, hit.ExplosionRadius, hit.Damage);
			SpawnExplosionEffect(registry, hit.ImpactPos, hit.ExplosionEffectPath, hit.ExplosionRadius, hit.VisualRadius);
		}

		for (entt::entity entity : hitProjectiles)
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
		std::vector<entt::entity> overlapped;
		::sys::PhysicsSystem::OverlapSphere(registry, center, radius, overlapped);

		for (entt::entity entity : overlapped)
		{
			if (!registry.all_of<ecs::EnemyTag>(entity)) continue;

			auto* status = registry.try_get<ecs::EnemyStatusComponent>(entity);
			if (status == nullptr) continue;

			// ノックバック等の物理的な反応はさせず、HPのみ減少させる
			// （HPが0以下になった後の破棄は EnemyDeathSystem が担当する）
			status->CurrentHp = std::max(0.0f, status->CurrentHp - damage);

			if (const auto* enemyTransform = registry.try_get<ecs::Transform>(entity))
			{
				ecs::combatutil::SpawnDamageNumber(enemyTransform->GetPosition(), damage, false);
			}
		}
	}

	/// <summary>着弾エフェクトを一度だけ再生する一時エンティティを生成する</summary>
	void ProjectileCollisionSystem::SpawnExplosionEffect(
		entt::registry& registry,
		const DirectX::XMFLOAT3& position,
		const std::string& effectPath,
		float hitRadius,
		float visualRadius)
	{
		// 実際の判定半径(hitRadius)を可視化する（ImGui「Physics Debug」→「Show Colliders」）。
		// HitRadiusMultiplierにより見た目(visualRadius)より大きくなっているため、
		// デバッグ表示は実際にダメージが及ぶ範囲(hitRadius)を優先して表示する。
		auto& manager = ::ecs::EntityManager::Get();
		auto wireEntity = manager.CreateEntity();
		auto& transform = manager.AddComponent<ecs::Transform>(wireEntity);
		transform.SetPosition(position);
		auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(wireEntity);
		wire.Radius = hitRadius;
		wire.Color = { 1.0f, 0.4f, 0.1f, 1.0f }; // 炎らしいオレンジ
		manager.AddComponent<ecs::TemporaryLifetimeComponent>(wireEntity).RemainingTime = kDebugWireLifetime;

		// 見た目のサイズは判定半径(hitRadius)ではなくvisualRadius基準で合わせる
		// （HitRadiusMultiplierで判定だけ拡大しても見た目は変えないため）。
		// effectPathは';'区切りで複数指定可能(ecs::effectutil::PlayOneShotCombined参照)。
		const float scale = visualRadius / kEffectReferenceRadius;
		ecs::effectutil::PlayOneShotCombined(effectPath, position, scale);
	}
}

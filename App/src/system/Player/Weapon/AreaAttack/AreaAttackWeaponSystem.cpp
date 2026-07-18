#include "apppch.h"
#include "AreaAttackWeaponSystem.h"

#include"AreaAttackWeaponRuntimeComponent.h"
#include"AreaAttackHazardComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/AimSysten/PlayerAimComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<Data/Weapon/AreaAttackWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<Tag/EntityTag.h>

namespace
{
	// エフェクト素材は概ねこの半径感で作られている想定の暫定値。
	// 実際の判定半径とのズレ(見た目は小さいのに判定は大きい/その逆)を軽減するための概算スケール。
	constexpr float kEffectReferenceRadius = 2.0f;
}

namespace ecs
{
	void AreaAttackWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// InGame中のみ発動する。必殺技演出中は他の攻撃を発動させない
		// (手動発動武器のためIsPlayerActionLocked()を使う。ecs::weaponutil::ShouldSkipManualWeaponUpdate参照)
		if (ecs::weaponutil::ShouldSkipManualWeaponUpdate(registry)) return;

		registry.view<ecs::WeaponComponent, ecs::AreaAttackWeaponRuntimeComponent>().each(
			[&](entt::entity weaponEntity, ecs::WeaponComponent& weapon, ecs::AreaAttackWeaponRuntimeComponent& runtime)
			{
				if (weapon.Type != ecs::eWeaponType::AreaAttack) return;
				if (!registry.valid(weapon.Owner)) return;

				const auto* masterData = DATA_MGR(data::AreaAttackWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
				if (masterData == nullptr) return;

				// 探索範囲(センサー)の可視化は発動可否・クールダウンに関係なく毎フレーム更新する
				UpdateSearchAreaVisual(registry, weaponEntity, weapon, *masterData);

				if (runtime.CooldownTimer > 0.0f)
				{
					runtime.CooldownTimer -= deltaTime;
				}
				if (runtime.CooldownTimer > 0.0f) return;

				// SingleShot(左クリック/Attack)と同じ初期武器のため、右クリック(Attack2)で発動する
				bool wantsToFire = weapon.Control == ecs::eWeaponControl::Auto;
				if (weapon.Control == ecs::eWeaponControl::Manual)
				{
					const auto* ownerAim = registry.try_get<ecs::PlayerAimComponent>(weapon.Owner);
					wantsToFire = ownerAim != nullptr && ownerAim->WantsToFireSecondary;
				}
				if (!wantsToFire) return;

				DEBUG_LOG(sys::eLogLevel::Log, "AreaAttackWeaponSystem: Fire triggered (control={})",
					weapon.Control == ecs::eWeaponControl::Auto ? "Auto" : "Manual");

				// 攻撃回数パーク(AttackCountUp)分だけ発動を繰り返す
				const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
				for (int i = 0; i < attackCount; ++i)
				{
					Fire(registry, weapon, *masterData);
				}

				runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
			});
	}

	/// <summary>探索範囲(センサー)を毎フレーム可視化する</summary>
	void AreaAttackWeaponSystem::UpdateSearchAreaVisual(
		entt::registry& registry,
		entt::entity weaponEntity,
		const ecs::WeaponComponent& weapon,
		const data::AreaAttackWeaponData& masterData)
	{
		const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
		const auto* ownerAim = registry.try_get<ecs::PlayerAimComponent>(weapon.Owner);
		if (ownerTransform == nullptr || ownerAim == nullptr) return;

		const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();
		const DirectX::XMFLOAT3& direction = ownerAim->Direction;

		const DirectX::XMFLOAT3 searchCenter =
		{
			ownerPos.x + direction.x * masterData.ForwardOffset,
			ownerPos.y,
			ownerPos.z + direction.z * masterData.ForwardOffset,
		};

		// 武器エンティティは本来Transformを持たないため、初回のみemplaceする
		auto* transform = registry.try_get<ecs::Transform>(weaponEntity);
		if (transform == nullptr)
		{
			transform = &registry.emplace<ecs::Transform>(weaponEntity);
		}
		transform->SetPosition(searchCenter);

		auto* wire = registry.try_get<ecs::DebugWireSphereComponent>(weaponEntity);
		if (wire == nullptr)
		{
			wire = &registry.emplace<ecs::DebugWireSphereComponent>(weaponEntity);
			wire->Color = { 0.2f, 0.9f, 1.0f, 0.5f }; // 探索範囲は控えめな水色
		}
		wire->Radius = masterData.SearchRadius;
	}

	/// <summary>狙い方向の範囲内から敵を検出し、各敵の座標へ氷柱(ハザード)を生成する</summary>
	void AreaAttackWeaponSystem::Fire(
		entt::registry& registry,
		const ecs::WeaponComponent& weapon,
		const data::AreaAttackWeaponData& masterData)
	{
		const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
		const auto* ownerAim = registry.try_get<ecs::PlayerAimComponent>(weapon.Owner);
		if (ownerTransform == nullptr || ownerAim == nullptr) return;

		const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();
		// 向いている方向 = マウス座標/右スティックでの狙い方向（SingleShotと同じ基準）
		const DirectX::XMFLOAT3& direction = ownerAim->Direction;

		const DirectX::XMFLOAT3 searchCenter =
		{
			ownerPos.x + direction.x * masterData.ForwardOffset,
			ownerPos.y,
			ownerPos.z + direction.z * masterData.ForwardOffset,
		};

		mFound.clear();
		::sys::PhysicsSystem::OverlapSphere(registry, searchCenter, masterData.SearchRadius, mFound);

		// AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
		const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
		const float damage = masterData.Damage * atkMultiplier;
		const float radius = masterData.Radius;

		int spawned = 0;
		for (entt::entity entity : mFound)
		{
			if (spawned >= masterData.MaxTargets) break;
			if (!registry.all_of<ecs::EnemyTag>(entity)) continue;

			const auto* enemyTransform = registry.try_get<ecs::Transform>(entity);
			if (enemyTransform == nullptr) continue;

			SpawnHazard(registry, enemyTransform->GetPosition(), radius, damage, masterData);
			++spawned;
		}

		DEBUG_LOG(sys::eLogLevel::Log, "AreaAttackWeaponSystem: found={} spawned={}", mFound.size(), spawned);
	}

	/// <summary>1体の敵の座標に氷柱(ハザード)エンティティを1体生成する</summary>
	void AreaAttackWeaponSystem::SpawnHazard(
		entt::registry& registry,
		const DirectX::XMFLOAT3& position,
		float radius,
		float damage,
		const data::AreaAttackWeaponData& masterData)
	{
		// 当たり判定半径は見た目基準半径(radius)とは別にHitRadiusMultiplierで拡大する。
		// エフェクトの見た目サイズは従来通りradius基準のままにするため、ここで分離する。
		const float hitRadius = radius * masterData.HitRadiusMultiplier;

		auto& manager = ::ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();

		auto& transform = manager.AddComponent<ecs::Transform>(entity);
		transform.SetPosition(position);

		auto& hazard = manager.AddComponent<ecs::AreaAttackHazardComponent>(entity);
		hazard.Radius = hitRadius;
		hazard.Damage = damage;
		hazard.RemainingDuration = masterData.Duration;
		hazard.TickTimer = 0.0f; // 0start: 生成した次のフレームで即座に1回目のダメージを与える
		hazard.TickInterval = masterData.TickInterval;

		// 実際の判定半径を可視化する（ImGui「Physics Debug」→「Show Colliders」）
		auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(entity);
		wire.Radius = hitRadius;
		wire.Color = { 0.4f, 0.8f, 1.0f, 1.0f }; // 氷らしい水色

		if (!masterData.EffectPath.empty())
		{
			auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
			effect.Asset = graphics::EffekseerManager::Get().GetEffect(masterData.EffectPath);
			// IsLoop=trueだと、素材の再生時間がDuration(5秒)より短い場合に「1回終わったら
			// 同じ場所にもう1発出た」ように見えてしまう(EffekseerManager::Updateが自動で再Play()する)。
			// 見た目は着弾時に1回だけ再生し、当たり判定(ダメージ反復)はAreaAttackHazardSystemが
			// Durationの間ずっと別途継続する（見た目の再生時間と判定の持続時間を分離する）。
			effect.IsLoop = false;
			// 見た目のサイズは判定半径(hitRadius)ではなくradius(見た目基準)で合わせる
			// （HitRadiusMultiplierで判定だけ拡大しても見た目は変えないため）
			const float scale = radius / kEffectReferenceRadius;
			effect.Scale = { scale, scale, scale };
			effect.Effect.Play(effect.Asset, position);
		}

		DEBUG_LOG(sys::eLogLevel::Log, "AreaAttackWeaponSystem: hazard entity={} spawned at ({}, {}, {}) hitRadius={} visualRadius={}",
			entt::to_integral(entity), position.x, position.y, position.z, hitRadius, radius);
	}
}

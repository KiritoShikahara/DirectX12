#include "apppch.h"
#include "AreaAttackWeaponSystem.h"

#include"AreaAttackWeaponRuntimeComponent.h"
#include"AreaAttackHazardComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/AimSysten/PlayerAimComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<Data/Weapon/AreaAttackWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<graphics/Line/Renderer/PhysicsDebugRenderer.h>
#include<Tag/EntityTag.h>

namespace
{
	// エフェクト素材のだいたいの基準半径。見た目のスケール計算に使う
	constexpr float kEffectReferenceRadius = 2.0f;
}

namespace ecs
{
	void AreaAttackWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// InGame中のみ発動。必殺技演出中は撃たせない
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
		// 判定半径は見た目のradiusとは別にHitRadiusMultiplierで拡大する
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

		// 実際の判定半径を可視化する（ImGui「Physics Debug」→「Show Colliders」）。
		// トグルOFF中は描画されず無駄なため、ONの時だけ生成する。
		if (graphics::PhysicsDebugRenderer::Get().IsEnabled())
		{
			auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(entity);
			wire.Radius = hitRadius;
			wire.Color = { 0.4f, 0.8f, 1.0f, 1.0f }; // 氷らしい水色
		}

		const std::string effectPath = ecs::effectutil::ResolveEffectIds(masterData.EffectIds);
		if (!effectPath.empty())
		{
			auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
			effect.Asset = graphics::EffekseerManager::Get().GetEffect(effectPath);
			// 見た目は着弾時に1回だけ。判定側の継続ダメージはAreaAttackHazardSystemが別に担う
			effect.IsLoop = false;
			// 見た目のスケールはradius基準(判定だけ拡大しても見た目は変えない)
			const float scale = radius / kEffectReferenceRadius;
			effect.Scale = { scale, scale, scale };
			effect.Effect.Play(effect.Asset, position);
			graphics::EffekseerManager::MarkSpawnHidden(effect);
		}

		DEBUG_LOG(sys::eLogLevel::Log, "AreaAttackWeaponSystem: hazard entity={} spawned at ({}, {}, {}) hitRadius={} visualRadius={}",
			entt::to_integral(entity), position.x, position.y, position.z, hitRadius, radius);
	}
}

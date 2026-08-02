#include "apppch.h"
#include "SingleShotWeaponSystem.h"

#include"SingleShotWeaponRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Weapon/Projectile/ProjectileComponent.h>
#include<system/Player/AimSysten/PlayerAimComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<Data/Weapon/SingleShotWeaponData.h>

namespace ecs
{
	void SingleShotWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// InGame中のみ発射する。必殺技演出中は他の攻撃を発動させない
		if (ecs::weaponutil::ShouldSkipManualWeaponUpdate(registry)) return;

		registry.view<ecs::WeaponComponent, ecs::SingleShotWeaponRuntimeComponent>().each(
			[&](ecs::WeaponComponent& weapon, ecs::SingleShotWeaponRuntimeComponent& runtime)
			{
				if (weapon.Type != ecs::eWeaponType::SingleShot) return;
				if (!registry.valid(weapon.Owner)) return;

				if (runtime.CooldownTimer > 0.0f)
				{
					runtime.CooldownTimer -= deltaTime;
				}
				if (runtime.CooldownTimer > 0.0f) return;

				// Auto: クールダウンが明けたら自動で撃つ / Manual: Attack入力が押された瞬間のみ撃つ
				bool wantsToFire = weapon.Control == ecs::eWeaponControl::Auto;
				if (weapon.Control == ecs::eWeaponControl::Manual)
				{
					const auto* ownerAim = registry.try_get<ecs::PlayerAimComponent>(weapon.Owner);
					wantsToFire = ownerAim != nullptr && ownerAim->WantsToFire;
				}
				if (!wantsToFire) return;

				const auto* masterData = DATA_MGR(data::SingleShotWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
				if (masterData == nullptr) return;

				// 攻撃回数パーク分だけ扇状に発射する
				const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
				for (int i = 0; i < attackCount; ++i)
				{
					Fire(registry, weapon, *masterData, i, attackCount);
				}

				runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
			});
	}

	namespace
	{
		constexpr float kMultiShotSpreadDegrees = 8.0f;
		constexpr float kFireSeVolume = 0.4f;
	}

	void SingleShotWeaponSystem::Fire(
		entt::registry& registry,
		const ecs::WeaponComponent& weapon,
		const data::SingleShotWeaponData& masterData,
		int shotIndex,
		int shotCount)
	{
		const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
		const auto* ownerAim = registry.try_get<ecs::PlayerAimComponent>(weapon.Owner);
		if (ownerTransform == nullptr || ownerAim == nullptr) return;

		PLAY_SE("Assets/Sound/SE/SE_Fire.aud", false, kFireSeVolume, false);

		const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();
		const DirectX::XMFLOAT3 direction = ecs::combatutil::ComputeSpreadDirection(
			ownerAim->Direction, shotIndex, shotCount, kMultiShotSpreadDegrees);

		// プレイヤー自身のコライダーに埋まって即着弾しないよう、狙い方向へ少し離した位置から発射する
		constexpr float kSpawnOffset = 1.0f;
		const DirectX::XMFLOAT3 spawnPos =
		{
			ownerPos.x + direction.x * kSpawnOffset,
			ownerPos.y + masterData.HeightOffset,
			ownerPos.z + direction.z * kSpawnOffset,
		};

		// AtkPowerパークの強化分をCurrent/Base比で反映する
		const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
		const float damage = masterData.Damage * atkMultiplier;
		const float radius = masterData.ExplosionRadius;
		// 当たり判定半径は見た目基準半径radiusとは別にHitRadiusMultiplierで拡大する
		const float hitRadius = radius * masterData.HitRadiusMultiplier;

		auto& manager = ::ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();

		auto& transform = manager.AddComponent<ecs::Transform>(entity);
		transform.SetPosition(spawnPos);

		manager.AddComponent<ecs::ColliderComponent>(entity, ecs::ColliderComponent::MakeSphere(0.3f));
		registry.emplace<ecs::SensorTagComponent>(entity);

		auto& rigid = manager.AddComponent<ecs::RigidBodyComponent>(entity, ecs::RigidBodyComponent::MakeKinematic());
		rigid.GravityFactor = 0.0f;

		auto& projectile = manager.AddComponent<ecs::ProjectileComponent>(entity);
		projectile.Direction = direction;
		projectile.Speed = masterData.ProjectileSpeed;
		projectile.Damage = damage;
		projectile.ExplosionRadius = hitRadius;
		projectile.VisualRadius = radius;
		projectile.ExplosionEffectPath = ecs::effectutil::ResolveEffectIds(masterData.ExplosionEffectIds);
		projectile.ExplosionAtGroundLevel = masterData.ExplosionAtGroundLevel;
		projectile.LifeTime = masterData.ProjectileLifeTime;
		projectile.Owner = weapon.Owner;

		const std::string projectileEffectPath = ecs::effectutil::ResolveEffectIds(masterData.ProjectileEffectIds);
		if (!projectileEffectPath.empty())
		{
			auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
			effect.Asset = graphics::EffekseerManager::Get().GetEffect(projectileEffectPath);
			effect.IsLoop = true;
			// 常に原点のeffect.Offsetではなく実際の発射位置を渡す、Transform追従が効くまでの1フレームのズレを防ぐため
			effect.Effect.Play(effect.Asset, spawnPos);
			graphics::EffekseerManager::MarkSpawnHidden(effect);
		}
	}
}

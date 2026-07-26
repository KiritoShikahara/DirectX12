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
		// (手動発動武器のためIsPlayerActionLocked()を使う。ecs::weaponutil::ShouldSkipManualWeaponUpdate参照)
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

				// Auto: クールダウンが明けたら自動で撃つ / Manual: "Attack"入力(左クリック)が押された瞬間のみ撃つ
				bool wantsToFire = weapon.Control == ecs::eWeaponControl::Auto;
				if (weapon.Control == ecs::eWeaponControl::Manual)
				{
					const auto* ownerAim = registry.try_get<ecs::PlayerAimComponent>(weapon.Owner);
					wantsToFire = ownerAim != nullptr && ownerAim->WantsToFire;
				}
				if (!wantsToFire) return;

				const auto* masterData = DATA_MGR(data::SingleShotWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
				if (masterData == nullptr) return;

				// 攻撃回数パーク(AttackCountUp)分だけ扇状に発射する
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
		// 攻撃回数パークで複数発射する際の、1ショットあたりの扇状スプレッド角度(度)
		constexpr float kMultiShotSpreadDegrees = 8.0f;
	}

	/// <summary>狙い方向(shotCount>1の場合は扇状に広げたshotIndex番目の方向)へ
	/// ProjectileComponent エンティティを1体生成する</summary>
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

		// AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
		const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
		const float damage = masterData.Damage * atkMultiplier;
		const float radius = masterData.ExplosionRadius;
		// 当たり判定半径は見た目基準半径(radius)とは別にHitRadiusMultiplierで拡大する。
		// エフェクトの見た目サイズは従来通りradius基準のままにするため、ここで分離する。
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
		projectile.LifeTime = masterData.ProjectileLifeTime;
		projectile.Owner = weapon.Owner;

		const std::string projectileEffectPath = ecs::effectutil::ResolveEffectIds(masterData.ProjectileEffectIds);
		if (!projectileEffectPath.empty())
		{
			auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
			effect.Asset = graphics::EffekseerManager::Get().GetEffect(projectileEffectPath);
			effect.IsLoop = true;
			// effect.Offset(常に原点)ではなく実際の発射位置を渡す。
			// ここを Offset のまま渡すと、次フレームの EffekseerManager::Update による
			// Transform追従が効くまでの1フレームだけ原点に表示されてしまう。
			effect.Effect.Play(effect.Asset, spawnPos);
			graphics::EffekseerManager::MarkSpawnHidden(effect);
		}
	}
}

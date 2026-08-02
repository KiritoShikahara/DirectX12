#include "apppch.h"
#include "OrbitWeaponSystem.h"

#include"OrbitWeaponRuntimeComponent.h"
#include"OrbitOrbComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Enemy/Status/EnemySlowStatusComponent.h>
#include<Data/Weapon/OrbitWeaponData.h>

#include<ecs/component/Debug/DebugWireSphereComponent.h>

namespace
{
	constexpr float kEffectReferenceRadius = 2.0f;
}

namespace ecs
{
	void OrbitWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// InGame中のみ動作する。必殺技演出中は既存オーブの周回・当たり判定も一時停止させる
		if (ecs::weaponutil::ShouldSkipAutoWeaponUpdate(registry)) return;

		registry.view<ecs::WeaponComponent, ecs::OrbitWeaponRuntimeComponent>().each(
			[&](ecs::WeaponComponent& weapon, ecs::OrbitWeaponRuntimeComponent& runtime)
			{
				if (weapon.Type != ecs::eWeaponType::SelfDefense) return;
				if (!registry.valid(weapon.Owner)) return;

				const auto* masterData = DATA_MGR(data::OrbitWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
				if (masterData == nullptr) return;

				UpdatePhase(registry, runtime, *masterData, deltaTime);
				if (!runtime.IsActive) return; // Cooldown中はオーブが存在しないため周回・判定をスキップ

				const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
				if (ownerTransform == nullptr) return;
				const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();
				const DirectX::XMFLOAT3 center =
				{
					ownerPos.x,
					ownerPos.y + masterData->HeightOffset,
					ownerPos.z,
				};

				// AtkPowerパークの強化分をCurrent/Base比で反映する
				const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
				const float damage = masterData->Damage * atkMultiplier;
				const float angularSpeed = DirectX::XMConvertToRadians(masterData->OrbitSpeed);

				for (entt::entity orb : runtime.Orbs)
				{
					if (!registry.valid(orb)) continue;

					UpdateOrbPosition(registry, orb, center, masterData->OrbitRadius, angularSpeed, deltaTime);
					ProcessOrbHit(registry, orb, damage, deltaTime, *masterData);
				}
			});
	}

	void OrbitWeaponSystem::UpdatePhase(
		entt::registry& registry,
		ecs::OrbitWeaponRuntimeComponent& runtime,
		const data::OrbitWeaponData& masterData,
		float deltaTime)
	{
		runtime.PhaseTimer -= deltaTime;
		if (runtime.PhaseTimer > 0.0f) return;

		if (runtime.IsActive)
		{
			// Active時間が終了：オーブを全消滅させ、Cooldownへ移行
			DespawnOrbs(registry, runtime);
			runtime.IsActive = false;
			runtime.PhaseTimer = masterData.CooldownDuration;
		}
		else
		{
			// Cooldownが終了：オーブを生成し、Activeへ移行
			SpawnOrbs(registry, runtime, masterData);
			runtime.IsActive = true;
			runtime.PhaseTimer = masterData.ActiveDuration;
		}
	}

	void OrbitWeaponSystem::SpawnOrbs(
		entt::registry& registry,
		ecs::OrbitWeaponRuntimeComponent& runtime,
		const data::OrbitWeaponData& masterData)
	{
		auto& manager = ::ecs::EntityManager::Get();
		const int orbCount = std::max(1, masterData.OrbCount);

		for (int i = 0; i < orbCount; ++i)
		{
			auto entity = manager.CreateEntity();

			// 実座標は次のUpdateOrbPositionで即座に正しい位置へ補正されるため、原点のままでよい
			manager.AddComponent<ecs::Transform>(entity);

			manager.AddComponent<ecs::ColliderComponent>(entity, ecs::ColliderComponent::MakeSphere(masterData.HitRadius));
			registry.emplace<ecs::SensorTagComponent>(entity);

			auto& rigid = manager.AddComponent<ecs::RigidBodyComponent>(entity, ecs::RigidBodyComponent::MakeKinematic());
			rigid.GravityFactor = 0.0f;

			auto& orb = manager.AddComponent<ecs::OrbitOrbComponent>(entity);
			// 均等配置、360度をOrbCountで割った角度をそれぞれの初期オフセットにする
			orb.Angle = (DirectX::XM_2PI / static_cast<float>(orbCount)) * static_cast<float>(i);
			orb.HitCooldownTimer = 0.0f;

			// 実際の当たり判定を可視化する
			auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(entity);
			wire.Radius = masterData.HitRadius;
			wire.Color = { 0.6f, 1.0f, 0.8f, 1.0f }; // 氷の欠片らしい淡い水色

			const std::string orbEffectPath = ecs::effectutil::ResolveEffectIds(masterData.OrbEffectIds);
			if (!orbEffectPath.empty())
			{
				auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
				effect.Asset = graphics::EffekseerManager::Get().GetEffect(orbEffectPath);
				effect.IsLoop = true; // 周回中はずっと表示し続ける持続エフェクト
				// 見た目のサイズを実際の判定半径に概算で合わせる、素材は概ねkEffectReferenceRadius相当と仮定
				const float scale = masterData.HitRadius / kEffectReferenceRadius;
				effect.Scale = { scale, scale, scale };
				effect.Effect.Play(effect.Asset, DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f });
				graphics::EffekseerManager::MarkSpawnHidden(effect);
			}

			runtime.Orbs.push_back(entity);
		}

		DEBUG_LOG(sys::eLogLevel::Log, "OrbitWeaponSystem: spawned {} orbs (orbitRadius={}, hitRadius={})",
			orbCount, masterData.OrbitRadius, masterData.HitRadius);
	}

	void OrbitWeaponSystem::DespawnOrbs(
		entt::registry& registry,
		ecs::OrbitWeaponRuntimeComponent& runtime)
	{
		// EffectComponentはデストラクタでEffekseerハンドルを自動停止するため、registry.destroyだけでよい
		for (entt::entity orb : runtime.Orbs)
		{
			if (registry.valid(orb)) registry.destroy(orb);
		}
		runtime.Orbs.clear();
	}

	void OrbitWeaponSystem::UpdateOrbPosition(
		entt::registry& registry,
		entt::entity orbEntity,
		const DirectX::XMFLOAT3& center,
		float orbitRadius,
		float angularSpeed,
		float deltaTime)
	{
		auto& orb = registry.get<ecs::OrbitOrbComponent>(orbEntity);
		orb.Angle += angularSpeed * deltaTime;
		if (orb.Angle > DirectX::XM_2PI) orb.Angle -= DirectX::XM_2PI; // 浮動小数点精度の劣化を防ぐ

		const DirectX::XMFLOAT3 position =
		{
			center.x + orbitRadius * std::cos(orb.Angle),
			center.y,
			center.z + orbitRadius * std::sin(orb.Angle),
		};

		auto& transform = registry.get<ecs::Transform>(orbEntity);
		transform.SetPosition(position);

		// Kinematic Bodyへ毎フレームの位置を反映させる、PhysicsSystem::SyncFromTransformが処理する
		registry.emplace_or_replace<ecs::TransformDirtyTag>(orbEntity);
	}

	void OrbitWeaponSystem::ProcessOrbHit(
		entt::registry& registry,
		entt::entity orbEntity,
		float damage,
		float deltaTime,
		const data::OrbitWeaponData& masterData)
	{
		auto& orb = registry.get<ecs::OrbitOrbComponent>(orbEntity);
		if (orb.HitCooldownTimer > 0.0f)
		{
			orb.HitCooldownTimer -= deltaTime;
			return;
		}

		// オーブは消滅しない持続武器のため、密着中は毎フレーム発行されるSensorStayEventを使う
		const auto* stay = registry.try_get<ecs::SensorStayEvent>(orbEntity);
		if (stay == nullptr) return; // このフレームは何にも触れていない

		bool hitAny = false;
		for (entt::entity other : stay->Visitors)
		{
			if (!registry.valid(other)) continue;

			if (ecs::combatutil::ApplyDamageToEnemy(registry, other, damage))
			{
				hitAny = true;

				// Frost効果、命中した敵を減速させる。emplace_or_replaceで多重付与ではなく更新にする
				if (masterData.SlowMultiplier < 1.0f && masterData.SlowDuration > 0.0f)
				{
					registry.emplace_or_replace<ecs::EnemySlowStatusComponent>(
						other,
						ecs::EnemySlowStatusComponent{ masterData.SlowMultiplier, masterData.SlowDuration });
				}
			}
		}

		if (!hitAny) return;

		orb.HitCooldownTimer = masterData.HitInterval;

		DEBUG_LOG(sys::eLogLevel::Log, "OrbitWeaponSystem: orb entity={} hit, damage={}",
			entt::to_integral(orbEntity), damage);

		const std::string hitEffectPath = ecs::effectutil::ResolveEffectIds(masterData.HitEffectIds);
		if (!hitEffectPath.empty())
		{
			const auto& orbTransform = registry.get<ecs::Transform>(orbEntity);
			SpawnHitEffect(registry, orbTransform.GetPosition(), hitEffectPath);
		}
	}

	void OrbitWeaponSystem::SpawnHitEffect(
		entt::registry& registry,
		const DirectX::XMFLOAT3& position,
		const std::string& effectPath)
	{
		auto& manager = ::ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();

		auto& transform = manager.AddComponent<ecs::Transform>(entity);
		transform.SetPosition(position);

		auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
		effect.Asset = graphics::EffekseerManager::Get().GetEffect(effectPath);
		effect.IsLoop = false;
		// autoDelete=true、再生終了フレームでEffekseerManager::Updateがこのエンティティを破棄する
		effect.Effect.Play(effect.Asset, position, true);
		graphics::EffekseerManager::MarkSpawnHidden(effect);
	}
}

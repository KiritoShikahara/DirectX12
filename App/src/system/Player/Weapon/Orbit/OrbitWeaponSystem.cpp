#include "apppch.h"
#include "OrbitWeaponSystem.h"

#include"OrbitWeaponRuntimeComponent.h"
#include"OrbitOrbComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<Data/Weapon/OrbitWeaponData.h>
#include<Scene/Game/State/GameState.h>

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
	void OrbitWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// InGame中のみ動作する（PerkSelect/Result中に発動し続けないようにする）
		auto stateView = registry.view<::ecs::GameStateComponent>();
		if (stateView.begin() == stateView.end()) return;
		if (registry.get<::ecs::GameStateComponent>(*stateView.begin()).GameState != ::sys::eGameState::InGame) return;
		// 必殺技演出中は既存オーブの周回・当たり判定も一時停止させる
		if (ecs::IsPlayerUltimateActive(registry)) return;

		registry.view<ecs::WeaponComponent, ecs::OrbitWeaponRuntimeComponent>().each(
			[&](ecs::WeaponComponent& weapon, ecs::OrbitWeaponRuntimeComponent& runtime)
			{
				if (weapon.Type != ecs::eWeaponType::SelfDefense) return;
				if (!registry.valid(weapon.Owner)) return;

				const auto* masterData = DATA_MGR(data::OrbitWeaponData).GetById(weapon.WeaponID);
				if (masterData == nullptr) return;

				// 発動トリガーの無い常時稼働の武器のため、初回Updateでオーブを生成する
				if (runtime.Orbs.empty())
				{
					SpawnOrbs(registry, runtime, *masterData);
				}

				const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
				if (ownerTransform == nullptr) return;
				const DirectX::XMFLOAT3& ownerPos = ownerTransform->GetPosition();
				const DirectX::XMFLOAT3 center =
				{
					ownerPos.x,
					ownerPos.y + masterData->HeightOffset,
					ownerPos.z,
				};

				// Lv1を基準（levelIndex=0）に、レベル毎の成長量を加算する。
				// AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
				const int levelIndex = std::max(0, weapon.Level - 1);
				const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
				const float damage = (masterData->BaseDamage + masterData->DamagePerLevel * static_cast<float>(levelIndex)) * atkMultiplier;
				const float angularSpeed = DirectX::XMConvertToRadians(masterData->OrbitSpeed);

				for (entt::entity orb : runtime.Orbs)
				{
					if (!registry.valid(orb)) continue;

					UpdateOrbPosition(registry, orb, center, masterData->OrbitRadius, angularSpeed, deltaTime);
					ProcessOrbHit(registry, orb, damage, deltaTime, *masterData);
				}
			});
	}

	/// <summary>OrbCount個のオーブエンティティを均等配置で生成する（初回のみ）</summary>
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
			// 均等配置：360度をOrbCountで割った角度をそれぞれの初期オフセットにする
			orb.Angle = (DirectX::XM_2PI / static_cast<float>(orbCount)) * static_cast<float>(i);
			orb.HitCooldownTimer = 0.0f;

			// 実際の当たり判定を可視化する（ImGui「Physics Debug」→「Show Colliders」）
			auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(entity);
			wire.Radius = masterData.HitRadius;
			wire.Color = { 0.6f, 1.0f, 0.8f, 1.0f }; // 氷の欠片らしい淡い水色

			if (!masterData.OrbEffectPath.empty())
			{
				auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
				effect.Asset = graphics::EffekseerManager::Get().GetEffect(masterData.OrbEffectPath);
				effect.IsLoop = true; // 周回中はずっと表示し続ける持続エフェクト
				// 見た目のサイズを実際の判定半径に概算で合わせる（素材は概ね kEffectReferenceRadius 相当と仮定）
				const float scale = masterData.HitRadius / kEffectReferenceRadius;
				effect.Scale = { scale, scale, scale };
				effect.Effect.Play(effect.Asset, DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f });
			}

			runtime.Orbs.push_back(entity);
		}

		DEBUG_LOG(sys::eLogLevel::Log, "OrbitWeaponSystem: spawned {} orbs (orbitRadius={}, hitRadius={})",
			orbCount, masterData.OrbitRadius, masterData.HitRadius);
	}

	/// <summary>周回角度を進め、中心座標を基準にオーブの位置を更新する</summary>
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

		// Kinematic Bodyへ毎フレームの位置を反映させる（PhysicsSystem::SyncFromTransformが処理する）
		registry.emplace_or_replace<ecs::TransformDirtyTag>(orbEntity);
	}

	/// <summary>SensorStayEventとヒットクールダウンを見て、接触中の敵全員にダメージを与える</summary>
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

		// オーブは消滅しない持続武器のため、侵入した瞬間にしか発行されないSensorEnterEventでは
		// 密着し続けた場合に再ダメージできない。密着中は毎フレーム発行されるSensorStayEventを使う
		// （PlayerContactDamageSystemの継続ダメージ判定と同じ方針）。
		const auto* stay = registry.try_get<ecs::SensorStayEvent>(orbEntity);
		if (stay == nullptr) return; // このフレームは何にも触れていない

		bool hitAny = false;
		for (entt::entity other : stay->Visitors)
		{
			if (!registry.valid(other)) continue;
			if (!registry.all_of<ecs::EnemyTag>(other)) continue;

			auto* status = registry.try_get<ecs::EnemyStatusComponent>(other);
			if (status == nullptr) continue;

			status->CurrentHp = std::max(0.0f, status->CurrentHp - damage);
			hitAny = true;

			if (const auto* enemyTransform = registry.try_get<ecs::Transform>(other))
			{
				ecs::combatutil::SpawnDamageNumber(enemyTransform->GetPosition(), damage, false);
			}
		}

		if (!hitAny) return;

		orb.HitCooldownTimer = masterData.HitInterval;

		DEBUG_LOG(sys::eLogLevel::Log, "OrbitWeaponSystem: orb entity={} hit, damage={}",
			entt::to_integral(orbEntity), damage);

		if (!masterData.HitEffectPath.empty())
		{
			const auto& orbTransform = registry.get<ecs::Transform>(orbEntity);
			SpawnHitEffect(registry, orbTransform.GetPosition(), masterData.HitEffectPath);
		}
	}

	/// <summary>命中時のワンショットエフェクトを1回再生する</summary>
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
		// autoDelete=true: 再生終了フレームでEffekseerManager::Updateがこのエンティティを破棄する
		effect.Effect.Play(effect.Asset, position, true);
	}
}

#include "apppch.h"
#include "MeteorWeaponSystem.h"

#include"MeteorWeaponRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<Data/Weapon/MeteorWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<graphics/Line/Renderer/PhysicsDebugRenderer.h>
#include<system/Effect/TemporaryLifetimeComponent.h>
#include<Tag/EntityTag.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<Scene/Game/Debug/GameDebugSettings.h>

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
	void MeteorWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// InGame中のみ発動する。必殺技演出中は他の攻撃を発動させない
		if (ecs::weaponutil::ShouldSkipAutoWeaponUpdate(registry)) return;

		registry.view<ecs::WeaponComponent, ecs::MeteorWeaponRuntimeComponent>().each(
			[&](ecs::WeaponComponent& weapon, ecs::MeteorWeaponRuntimeComponent& runtime)
			{
				if (weapon.Type != ecs::eWeaponType::Meteor) return;
				if (!registry.valid(weapon.Owner)) return;

				if (runtime.CooldownTimer > 0.0f)
				{
					runtime.CooldownTimer -= deltaTime;
				}
				if (runtime.CooldownTimer > 0.0f) return;

				const auto* masterData = DATA_MGR(data::MeteorWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
				if (masterData == nullptr) return;

				// SearchRadius内に敵が1体も見つからなければクールダウンを消費せず待機する、空撃ち防止
				if (!Fire(registry, weapon, *masterData)) return;

				// 攻撃回数パーク分だけ追加で発動を繰り返す、1回目は上のFireで消費済み
				const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
				for (int i = 1; i < attackCount; ++i)
				{
					Fire(registry, weapon, *masterData);
				}

				runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
			});
	}

	bool MeteorWeaponSystem::Fire(
		entt::registry& registry,
		const ecs::WeaponComponent& weapon,
		const data::MeteorWeaponData& masterData)
	{
		const auto* ownerTransform = registry.try_get<ecs::Transform>(weapon.Owner);
		if (ownerTransform == nullptr) return false;

		mFound.clear();
		::sys::PhysicsSystem::OverlapSphere(registry, ownerTransform->GetPosition(), masterData.SearchRadius, mFound);

		mEnemies.clear();
		mEnemies.reserve(mFound.size());
		for (entt::entity entity : mFound)
		{
			if (registry.all_of<ecs::EnemyTag>(entity) && registry.all_of<ecs::Transform>(entity))
			{
				mEnemies.push_back(entity);
			}
		}
		if (mEnemies.empty()) return false;

		// 重複無しでランダムにMeteorCount体まで選ぶ
		std::shuffle(mEnemies.begin(), mEnemies.end(), GetRandomEngine());
		const int count = std::min<int>(masterData.MeteorCount, static_cast<int>(mEnemies.size()));

		// AtkPowerパークの強化分をCurrent/Base比で反映する
		const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
		const float damage = masterData.Damage * atkMultiplier;
		const float radius = masterData.Radius;
		// 当たり判定半径は見た目基準半径radiusとは別にHitRadiusMultiplierで拡大する
		const float hitRadius = radius * masterData.HitRadiusMultiplier;

		for (int i = 0; i < count; ++i)
		{
			const auto& targetTransform = registry.get<ecs::Transform>(mEnemies[i]);
			const DirectX::XMFLOAT3& targetPos = targetTransform.GetPosition();
			const DirectX::XMFLOAT3 strikePos =
			{
				targetPos.x,
				targetPos.y + masterData.HeightOffset,
				targetPos.z,
			};

			Strike(registry, strikePos, hitRadius, damage, radius, masterData);
		}

		return true;
	}

	void MeteorWeaponSystem::Strike(
		entt::registry& registry,
		const DirectX::XMFLOAT3& position,
		float hitRadius,
		float damage,
		float visualRadius,
		const data::MeteorWeaponData& masterData)
	{
		mOverlapped.clear();
		::sys::PhysicsSystem::OverlapSphere(registry, position, hitRadius, mOverlapped);

		for (entt::entity entity : mOverlapped)
		{
			ecs::combatutil::ApplyDamageToEnemy(registry, entity, damage);
		}

		// 実際の判定半径を可視化する、付け忘れると永久に残り続けるためTemporaryLifetimeComponentで明示的に破棄する
		if (graphics::PhysicsDebugRenderer::Get().IsEnabled())
		{
			auto& manager = ::ecs::EntityManager::Get();
			auto wireEntity = manager.CreateEntity();
			auto& transform = manager.AddComponent<ecs::Transform>(wireEntity);
			transform.SetPosition(position);
			auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(wireEntity);
			wire.Radius = hitRadius;
			wire.Color = { 1.0f, 0.5f, 0.1f, 1.0f }; // 隕石らしいオレンジ
			manager.AddComponent<ecs::TemporaryLifetimeComponent>(wireEntity).RemainingTime = kDebugWireLifetime;
		}

		// 見た目のサイズは判定半径hitRadiusではなくvisualRadius基準に合わせる
		const float scale = visualRadius / kEffectReferenceRadius;
		const std::string effectPath = ecs::effectutil::ResolveEffectIds(masterData.EffectIds);
		ecs::effectutil::PlayOneShotCombined(effectPath, position, scale);
	}
}

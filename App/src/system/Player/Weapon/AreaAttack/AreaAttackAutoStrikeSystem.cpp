#include "apppch.h"
#include "AreaAttackAutoStrikeSystem.h"

#include"AreaAttackWeaponRuntimeComponent.h"
#include"AreaAttackHazardSpawner.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<Data/Weapon/AreaAttackWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<Tag/EntityTag.h>
#include<Scene/Game/Debug/GameDebugSettings.h>

#include<algorithm>
#include<random>

namespace
{
	std::mt19937& GetRandomEngine()
	{
		// プロセス全体で1つの乱数エンジンを使い回す、毎フレーム再生成しない
		static std::mt19937 engine = ::debug::GameDebugSettings::Get().MakeRandomEngine();
		return engine;
	}
}

namespace ecs
{
	void AreaAttackAutoStrikeSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// InGame中のみ発動する。必殺技演出中は他の攻撃を発動させない
		if (ecs::weaponutil::ShouldSkipAutoWeaponUpdate(registry)) return;

		registry.view<ecs::WeaponComponent, ecs::AreaAttackWeaponRuntimeComponent>().each(
			[&](ecs::WeaponComponent& weapon, ecs::AreaAttackWeaponRuntimeComponent& runtime)
			{
				if (weapon.Type != ecs::eWeaponType::AreaAttack) return;
				if (!registry.valid(weapon.Owner)) return;

				// 手動発動用のCooldownTimerとは完全に独立したタイマーを使う
				if (runtime.AutoStrikeCooldownTimer > 0.0f)
				{
					runtime.AutoStrikeCooldownTimer -= deltaTime;
				}
				if (runtime.AutoStrikeCooldownTimer > 0.0f) return;

				const auto* masterData = DATA_MGR(data::AreaAttackWeaponData).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
				if (masterData == nullptr) return;

				// SearchRadius内に敵が1体も見つからなければクールダウンを消費せず待機する、空撃ち防止
				if (!Fire(registry, weapon, *masterData)) return;

				runtime.AutoStrikeCooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
			});
	}

	bool AreaAttackAutoStrikeSystem::Fire(
		entt::registry& registry,
		const ecs::WeaponComponent& weapon,
		const data::AreaAttackWeaponData& masterData)
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

		// 重複無しでランダムにAutoStrikeCount体まで選ぶ
		std::shuffle(mEnemies.begin(), mEnemies.end(), GetRandomEngine());
		const int count = std::min<int>(masterData.AutoStrikeCount, static_cast<int>(mEnemies.size()));

		// ダメージ/半径は手動発動のAreaAttackWeaponSystem::Fireと全く同じ値を使う
		const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
		const float damage = masterData.Damage * atkMultiplier;
		const float radius = masterData.Radius;

		for (int i = 0; i < count; ++i)
		{
			const auto& targetTransform = registry.get<ecs::Transform>(mEnemies[i]);
			ecs::areaattack::SpawnHazard(registry, targetTransform.GetPosition(), radius, damage, masterData);
		}

		DEBUG_LOG(sys::eLogLevel::Log, "AreaAttackAutoStrikeSystem: found={} struck={}", mEnemies.size(), count);

		return true;
	}
}

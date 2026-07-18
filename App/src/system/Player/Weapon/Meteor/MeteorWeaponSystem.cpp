#include "apppch.h"
#include "MeteorWeaponSystem.h"

#include"MeteorWeaponRuntimeComponent.h"
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<Data/Weapon/MeteorWeaponData.h>

#include<system/Physics/System/PhysicsSystem.h>
#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<system/Effect/TemporaryLifetimeComponent.h>
#include<Tag/EntityTag.h>
#include<system/Effect/EffectSpawnUtility.h>

#include<algorithm>
#include<random>

namespace
{
	// エフェクト素材は概ねこの半径感で作られている想定の暫定値(他の武器と同じ基準)。
	constexpr float kEffectReferenceRadius = 2.0f;

	// 判定半径可視化用ワイヤーの表示時間(秒)。
	constexpr float kDebugWireLifetime = 0.3f;

	// プロセス全体で1つの乱数エンジンを使い回す（毎フレーム再生成しない）
	std::mt19937& GetRandomEngine()
	{
		static std::mt19937 engine{ std::random_device{}() };
		return engine;
	}
}

namespace ecs
{
	void MeteorWeaponSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// InGame中のみ発動する。必殺技演出中は他の攻撃を発動させない(自動発動武器のため
		// Flicker Strike中は止めない設計。ecs::weaponutil::ShouldSkipAutoWeaponUpdate参照)
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

				// SearchRadius内に敵が1体も見つからなければクールダウンを消費せず待機する
				// （対象なしで空撃ちしないため。他の自動発動武器と同じ方針）
				if (!Fire(registry, weapon, *masterData)) return;

				// 攻撃回数パーク(AttackCountUp)分だけ追加で発動を繰り返す(1回目は上のFireで消費済み)
				const int attackCount = ecs::combatutil::GetAttackCount(registry, weapon.Owner);
				for (int i = 1; i < attackCount; ++i)
				{
					Fire(registry, weapon, *masterData);
				}

				runtime.CooldownTimer = masterData->FireInterval * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
			});
	}

	/// <summary>発動: SearchRadius内の敵からランダムに最大MeteorCount体を選び、
	/// それぞれの座標へ隕石(範囲ダメージ+エフェクト)を落とす</summary>
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

		// AtkPowerパークの強化分をCurrent/Base比で反映する(ecs::combatutil参照)
		const float atkMultiplier = ecs::combatutil::GetAtkPowerMultiplier(registry, weapon.Owner);
		const float damage = masterData.Damage * atkMultiplier;
		const float radius = masterData.Radius;
		// 当たり判定半径は見た目基準半径(radius)とは別にHitRadiusMultiplierで拡大する。
		// エフェクトの見た目サイズは従来通りradius基準のままにするため、ここで分離する。
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

	/// <summary>1体分の隕石落下：範囲ダメージを与えワンショットエフェクトを再生する</summary>
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

		// 実際の判定半径(hitRadius)を可視化する（ImGui「Physics Debug」→「Show Colliders」）。
		// EffectComponentのautoDeleteに乗らないため、TemporaryLifetimeComponentで
		// 明示的に一定時間後に破棄する（付け忘れると永久に残り続けるバグになる）。
		auto& manager = ::ecs::EntityManager::Get();
		auto wireEntity = manager.CreateEntity();
		auto& transform = manager.AddComponent<ecs::Transform>(wireEntity);
		transform.SetPosition(position);
		auto& wire = manager.AddComponent<ecs::DebugWireSphereComponent>(wireEntity);
		wire.Radius = hitRadius;
		wire.Color = { 1.0f, 0.5f, 0.1f, 1.0f }; // 隕石らしいオレンジ
		manager.AddComponent<ecs::TemporaryLifetimeComponent>(wireEntity).RemainingTime = kDebugWireLifetime;

		// 見た目のサイズは判定半径(hitRadius)ではなくvisualRadius(見た目基準)に合わせる。
		// EffectPathは';'区切りで複数指定可能(ecs::effectutil::PlayOneShotCombined参照)。
		const float scale = visualRadius / kEffectReferenceRadius;
		ecs::effectutil::PlayOneShotCombined(masterData.EffectPath, position, scale);
	}
}

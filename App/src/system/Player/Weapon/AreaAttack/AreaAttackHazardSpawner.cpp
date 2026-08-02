#include "apppch.h"
#include "AreaAttackHazardSpawner.h"

#include"AreaAttackHazardComponent.h"
#include<system/Effect/EffectSpawnUtility.h>
#include<Data/Weapon/AreaAttackWeaponData.h>

#include<ecs/component/Debug/DebugWireSphereComponent.h>
#include<graphics/Line/Renderer/PhysicsDebugRenderer.h>

namespace
{
	constexpr float kEffectReferenceRadius = 2.0f;
	constexpr float kVisualScaleMultiplier = 0.5f;
	constexpr float kStrikeSeVolume = 0.2f;
}

namespace ecs::areaattack
{
	void SpawnHazard(
		entt::registry& registry,
		const DirectX::XMFLOAT3& position,
		float radius,
		float damage,
		const data::AreaAttackWeaponData& masterData)
	{
		PLAY_SE("Assets/Sound/SE/SE_Area.aud", false, kStrikeSeVolume, false);

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
		hazard.TickTimer = 0.0f; // 0スタート、生成した次のフレームで即座に1回目のダメージを与える
		hazard.TickInterval = masterData.TickInterval;

		// 実際の判定半径を可視化する、トグルONの時だけ生成する
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
			// 見た目のスケールはradius基準、判定だけ拡大しても見た目は変えない
			const float scale = (radius / kEffectReferenceRadius) * kVisualScaleMultiplier;
			effect.Scale = { scale, scale, scale };
			effect.Effect.Play(effect.Asset, position);
			graphics::EffekseerManager::MarkSpawnHidden(effect);
		}

		DEBUG_LOG(sys::eLogLevel::Log, "AreaAttackHazardSpawner: hazard entity={} spawned at ({}, {}, {}) hitRadius={} visualRadius={}",
			entt::to_integral(entity), position.x, position.y, position.z, hitRadius, radius);
	}
}

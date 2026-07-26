#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct OrbitWeaponData; }

namespace ecs
{
	struct WeaponComponent;
	struct OrbitWeaponRuntimeComponent;

	/// <summary>
	/// SelfDefense(周回)型武器(WeaponComponent::Type == SelfDefense)のロジック。
	/// 発動トリガーは無いが常時稼働ではなく、ActiveDuration(秒)出現→CooldownDuration(秒)消滅を
	/// 繰り返すサイクル制で、Active中はOrbCount個のオーブが所有者(Owner)を中心に
	/// OrbitRadius(m)の円周上をOrbitSpeed(度/秒)で周回する。
	/// 各オーブはSensorStayEvent(密着中は毎フレーム発行)を使い、HitInterval(秒)ごとに
	/// 接触中の敵全員へダメージを与え、SpeedMultiplier倍の減速(EnemySlowStatusComponent)を
	/// SlowDuration(秒)だけ付与する（オーブ自体は消滅しない持続当たり判定のため、
	/// 1回しか発行されないSensorEnterEventでは密着し続けた場合に再ダメージできない）。
	/// ダメージは OrbitWeaponData(マスタ) と WeaponComponent::Level から算出する。
	/// </summary>
	class OrbitWeaponSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>
		/// Active/Cooldownのフェーズ残り時間を進め、尽きていればフェーズを切り替える
		/// (Active満了→オーブ全消滅してCooldownへ、Cooldown満了→オーブ生成してActiveへ)。
		/// </summary>
		static void UpdatePhase(
			entt::registry& registry,
			ecs::OrbitWeaponRuntimeComponent& runtime,
			const data::OrbitWeaponData& masterData,
			float deltaTime);

		/// <summary>OrbCount個のオーブエンティティを均等配置で生成する</summary>
		static void SpawnOrbs(
			entt::registry& registry,
			ecs::OrbitWeaponRuntimeComponent& runtime,
			const data::OrbitWeaponData& masterData);

		/// <summary>周回中のオーブエンティティを全て破棄する(Cooldown移行時)</summary>
		static void DespawnOrbs(
			entt::registry& registry,
			ecs::OrbitWeaponRuntimeComponent& runtime);

		/// <summary>周回角度を進め、中心座標を基準にオーブの位置を更新する</summary>
		static void UpdateOrbPosition(
			entt::registry& registry,
			entt::entity orbEntity,
			const DirectX::XMFLOAT3& center,
			float orbitRadius,
			float angularSpeed,
			float deltaTime);

		/// <summary>SensorStayEventとヒットクールダウンを見て、接触中の敵全員にダメージ+減速を与える</summary>
		static void ProcessOrbHit(
			entt::registry& registry,
			entt::entity orbEntity,
			float damage,
			float deltaTime,
			const data::OrbitWeaponData& masterData);

		/// <summary>命中時のワンショットエフェクトを1回再生する</summary>
		static void SpawnHitEffect(
			entt::registry& registry,
			const DirectX::XMFLOAT3& position,
			const std::string& effectPath);
	};
}

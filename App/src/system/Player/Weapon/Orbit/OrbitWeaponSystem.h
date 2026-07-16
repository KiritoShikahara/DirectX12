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
	/// 発動トリガーは無く、装備した瞬間からOrbCount個のオーブを生成し、
	/// 所有者(Owner)を中心にOrbitRadius(m)の円周上をOrbitSpeed(度/秒)で周回させ続ける。
	/// 各オーブはSensorStayEvent(密着中は毎フレーム発行)を使い、HitInterval(秒)ごとに
	/// 接触中の敵全員へダメージを与える（オーブ自体は消滅しない持続武器のため、
	/// 1回しか発行されないSensorEnterEventでは密着し続けた場合に再ダメージできない）。
	/// ダメージは OrbitWeaponData(マスタ) と WeaponComponent::Level から算出する。
	/// </summary>
	class OrbitWeaponSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>OrbCount個のオーブエンティティを均等配置で生成する（初回のみ）</summary>
		static void SpawnOrbs(
			entt::registry& registry,
			ecs::OrbitWeaponRuntimeComponent& runtime,
			const data::OrbitWeaponData& masterData);

		/// <summary>周回角度を進め、中心座標を基準にオーブの位置を更新する</summary>
		static void UpdateOrbPosition(
			entt::registry& registry,
			entt::entity orbEntity,
			const DirectX::XMFLOAT3& center,
			float orbitRadius,
			float angularSpeed,
			float deltaTime);

		/// <summary>SensorStayEventとヒットクールダウンを見て、接触中の敵全員にダメージを与える</summary>
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

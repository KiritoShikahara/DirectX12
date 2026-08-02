#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>
#include<ecs/system/manager/IComponentSystem.h>

namespace data { struct OrbitWeaponData; }

namespace ecs
{
	struct WeaponComponent;
	struct OrbitWeaponRuntimeComponent;

	///<summary>
	///SelfDefense型武器のロジック。ActiveDuration出現・CooldownDuration消滅を繰り返し、Active中はオーブが周回して継続ダメージを与える
	///</summary>
	class OrbitWeaponSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///Active/Cooldownのフェーズ残り時間を進め、尽きていればフェーズを切り替える
		///</summary>
		static void UpdatePhase(
			entt::registry& registry,
			ecs::OrbitWeaponRuntimeComponent& runtime,
			const data::OrbitWeaponData& masterData,
			float deltaTime);

		///<summary>
		///OrbCount個のオーブエンティティを均等配置で生成する
		///</summary>
		static void SpawnOrbs(
			entt::registry& registry,
			ecs::OrbitWeaponRuntimeComponent& runtime,
			const data::OrbitWeaponData& masterData);

		///<summary>
		///周回中のオーブエンティティを全て破棄する、Cooldown移行時
		///</summary>
		static void DespawnOrbs(
			entt::registry& registry,
			ecs::OrbitWeaponRuntimeComponent& runtime);

		///<summary>
		///周回角度を進め、中心座標を基準にオーブの位置を更新する
		///</summary>
		static void UpdateOrbPosition(
			entt::registry& registry,
			entt::entity orbEntity,
			const DirectX::XMFLOAT3& center,
			float orbitRadius,
			float angularSpeed,
			float deltaTime);

		///<summary>
		///SensorStayEventとヒットクールダウンを見て、接触中の敵全員にダメージ・減速を与える
		///</summary>
		static void ProcessOrbHit(
			entt::registry& registry,
			entt::entity orbEntity,
			float damage,
			float deltaTime,
			const data::OrbitWeaponData& masterData);

		///<summary>
		///命中時のワンショットエフェクトを1回再生する
		///</summary>
		static void SpawnHitEffect(
			entt::registry& registry,
			const DirectX::XMFLOAT3& position,
			const std::string& effectPath);
	};
}

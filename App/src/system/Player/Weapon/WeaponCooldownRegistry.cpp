#include "apppch.h"
#include "WeaponCooldownRegistry.h"

#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponUpdateUtil.h>
#include<system/Player/Status/PlayerCombatUtil.h>

#include<system/Player/Weapon/SingleShot/SingleShotWeaponRuntimeComponent.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Nova/NovaWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Homing/HomingMissileRuntimeComponent.h>
#include<system/Player/Weapon/ChainLightning/ChainLightningRuntimeComponent.h>
#include<system/Player/Weapon/Meteor/MeteorWeaponRuntimeComponent.h>
#include<system/Player/Weapon/VoidBeam/VoidBeamRuntimeComponent.h>
#include<system/Player/Weapon/BoneSpear/BoneSpearRuntimeComponent.h>
#include<system/Player/Weapon/Cleave/CleaveRuntimeComponent.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeRuntimeComponent.h>
#include<system/Player/Weapon/Ricochet/RicochetRuntimeComponent.h>

#include<Data/Weapon/SingleShotWeaponData.h>
#include<Data/Weapon/AreaAttackWeaponData.h>
#include<Data/Weapon/NovaWeaponData.h>
#include<Data/Weapon/HomingMissileWeaponData.h>
#include<Data/Weapon/ChainLightningWeaponData.h>
#include<Data/Weapon/MeteorWeaponData.h>
#include<Data/Weapon/VoidBeamWeaponData.h>
#include<Data/Weapon/BoneSpearWeaponData.h>
#include<Data/Weapon/CleaveWeaponData.h>
#include<Data/Weapon/FlickerStrikeWeaponData.h>
#include<Data/Weapon/RicochetWeaponData.h>

#include<functional>
#include<unordered_map>

namespace
{
	using GetCooldownFn = std::function<bool(
		entt::registry&, entt::entity, const ecs::WeaponComponent&, float&, float&)>;

	/// <summary>RuntimeComponentの残り秒数とマスタデータのFireInterval系フィールドから
	/// (残り, 最大)を求める定型処理。武器種別ごとの差異はテンプレート引数とラムダに閉じ込める</summary>
	template<typename RuntimeT, typename MasterDataT, typename GetIntervalFn>
	bool GetCooldownGeneric(
		entt::registry& registry, entt::entity weaponEntity, const ecs::WeaponComponent& weapon,
		float& outRemaining, float& outMax, GetIntervalFn getInterval)
	{
		const auto* runtime = registry.try_get<RuntimeT>(weaponEntity);
		if (runtime == nullptr) return false;

		const auto* masterData = DATA_MGR(MasterDataT).GetById(ecs::weaponutil::ComputeWeaponDataId(weapon));
		if (masterData == nullptr) return false;

		outRemaining = runtime->CooldownTimer;
		outMax = getInterval(*masterData) * ecs::combatutil::GetCooldownRate(registry, weapon.Owner);
		return true;
	}

	/// <summary>武器種別→クールダウン取得関数のテーブル。Orbitは常時稼働でクールダウンが
	/// 存在しないため意図的に登録しない(TryGetWeaponCooldownはfalseを返す)</summary>
	const std::unordered_map<ecs::eWeaponType, GetCooldownFn>& GetRegistry()
	{
		static const std::unordered_map<ecs::eWeaponType, GetCooldownFn> table =
		{
			{ ecs::eWeaponType::SingleShot, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::SingleShotWeaponRuntimeComponent, data::SingleShotWeaponData>(
					r, e, w, rem, mx, [](const data::SingleShotWeaponData& d) { return d.FireInterval; }); } },

			{ ecs::eWeaponType::AreaAttack, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::AreaAttackWeaponRuntimeComponent, data::AreaAttackWeaponData>(
					r, e, w, rem, mx, [](const data::AreaAttackWeaponData& d) { return d.FireInterval; }); } },

			{ ecs::eWeaponType::Nova, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::NovaWeaponRuntimeComponent, data::NovaWeaponData>(
					r, e, w, rem, mx, [](const data::NovaWeaponData& d) { return d.PulseInterval; }); } },

			{ ecs::eWeaponType::Homing, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::HomingMissileRuntimeComponent, data::HomingMissileWeaponData>(
					r, e, w, rem, mx, [](const data::HomingMissileWeaponData& d) { return d.FireInterval; }); } },

			{ ecs::eWeaponType::Chain, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::ChainLightningRuntimeComponent, data::ChainLightningWeaponData>(
					r, e, w, rem, mx, [](const data::ChainLightningWeaponData& d) { return d.FireInterval; }); } },

			{ ecs::eWeaponType::Meteor, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::MeteorWeaponRuntimeComponent, data::MeteorWeaponData>(
					r, e, w, rem, mx, [](const data::MeteorWeaponData& d) { return d.FireInterval; }); } },

			{ ecs::eWeaponType::VoidBeam, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::VoidBeamRuntimeComponent, data::VoidBeamWeaponData>(
					r, e, w, rem, mx, [](const data::VoidBeamWeaponData& d) { return d.FireInterval; }); } },

			{ ecs::eWeaponType::BoneSpear, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::BoneSpearRuntimeComponent, data::BoneSpearWeaponData>(
					r, e, w, rem, mx, [](const data::BoneSpearWeaponData& d) { return d.FireInterval; }); } },

			{ ecs::eWeaponType::Cleave, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::CleaveRuntimeComponent, data::CleaveWeaponData>(
					r, e, w, rem, mx, [](const data::CleaveWeaponData& d) { return d.FireInterval; }); } },

			{ ecs::eWeaponType::FlickerStrike, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::FlickerStrikeRuntimeComponent, data::FlickerStrikeWeaponData>(
					r, e, w, rem, mx, [](const data::FlickerStrikeWeaponData& d) { return d.FireInterval; }); } },

			{ ecs::eWeaponType::Ricochet, [](entt::registry& r, entt::entity e, const ecs::WeaponComponent& w, float& rem, float& mx)
				{ return GetCooldownGeneric<ecs::RicochetRuntimeComponent, data::RicochetWeaponData>(
					r, e, w, rem, mx, [](const data::RicochetWeaponData& d) { return d.FireInterval; }); } },
		};
		return table;
	}
}

namespace ecs::weaponutil
{
	bool TryGetWeaponCooldown(
		entt::registry& registry,
		entt::entity weaponEntity,
		const WeaponComponent& weapon,
		float& outRemainingSeconds,
		float& outMaxSeconds)
	{
		const auto& table = GetRegistry();
		const auto it = table.find(weapon.Type);
		if (it == table.end()) return false;

		return it->second(registry, weaponEntity, weapon, outRemainingSeconds, outMaxSeconds);
	}
}

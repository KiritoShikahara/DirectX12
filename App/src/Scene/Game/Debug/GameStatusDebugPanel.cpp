#include "apppch.h"
#include "GameStatusDebugPanel.h"

#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Level/PlayerLevelComponent.h>
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/SingleShot/SingleShotWeaponRuntimeComponent.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Orbit/OrbitWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Nova/NovaWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Homing/HomingMissileRuntimeComponent.h>
#include<system/Player/Weapon/ChainLightning/ChainLightningRuntimeComponent.h>
#include<system/Player/Weapon/VoidBeam/VoidBeamRuntimeComponent.h>
#include<system/Player/Weapon/BoneSpear/BoneSpearRuntimeComponent.h>
#include<system/Player/Weapon/Cleave/CleaveRuntimeComponent.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeRuntimeComponent.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeComponent.h>
#include<system/Player/PowerCharge/PlayerPowerChargeComponent.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<Data/Ultimate/UltimateData.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Tag/EntityTag.h>
#include<Scene/Game/Wave/WaveComponent.h>

namespace debug
{
	GameStatusDebugPanel::GameStatusDebugPanel(std::string debugKey)
		: mDebugKey(std::move(debugKey))
	{
#ifdef _DEBUG
		sys::ImGuiManager::Get().AddDebugUI([this]() { Draw(); }, mDebugKey);
#endif
	}

	GameStatusDebugPanel::~GameStatusDebugPanel()
	{
#ifdef _DEBUG
		sys::ImGuiManager::Get().RemoveDebugUI(mDebugKey);
#endif
	}

#ifdef _DEBUG
	void GameStatusDebugPanel::Draw()
	{
		if (!ImGui::Begin("Game Status"))
		{
			ImGui::End();
			return;
		}

		auto& registry = ecs::EntityManager::Get().GetRegistry();

		if (ImGui::CollapsingHeader("Wave", ImGuiTreeNodeFlags_DefaultOpen))
		{
			registry.view<ecs::WaveComponent>().each(
				[&](ecs::WaveComponent& wave)
				{
					ImGui::Text("Elapsed: %.1f s", wave.ElapsedTime);
					ImGui::Text("Next spawn in: %.1f s", wave.SpawnTimer);
					if (wave.BossSpawned)
					{
						ImGui::Text("Boss: spawned");
					}
					else
					{
						ImGui::Text("Boss in: %.1f s", wave.BossSpawnTime - wave.ElapsedTime);
					}
					ImGui::Text("Clear in: %.1f s", wave.ClearTime - wave.ElapsedTime);
				});
		}

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen))
		{
			registry.view<ecs::PlayerTag, ecs::PlayerStatusComponent>().each(
				[&](ecs::PlayerStatusComponent& status)
				{
					const float maxHp = status.Current.MaxHp;
					const float ratio = maxHp > 0.0f ? status.CurrentHp / maxHp : 0.0f;

					ImGui::Text("HP: %.1f / %.1f", status.CurrentHp, maxHp);
					ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f));
					ImGui::Text("AtkPower: %.1f   Defense: %.1f", status.Current.AtkPower, status.Current.Defense);
					ImGui::Text("MoveSpeed: %.1f   CooldownRate: %.2f", status.Current.MoveSpeed, status.Current.CooldownRate);
				});

			registry.view<ecs::PlayerTag, ecs::PlayerLevelComponent>().each(
				[&](ecs::PlayerLevelComponent& level)
				{
					const float xpRatio = level.ExperienceToNextLevel > 0.0f
						? level.Experience / level.ExperienceToNextLevel
						: 0.0f;

					ImGui::Text("Level: %d", level.Level);
					ImGui::Text("EXP: %.1f / %.1f", level.Experience, level.ExperienceToNextLevel);
					ImGui::ProgressBar(xpRatio, ImVec2(-1.0f, 0.0f));
				});

			registry.view<ecs::PlayerTag, ecs::PlayerUltimateComponent>().each(
				[&](ecs::PlayerUltimateComponent& ultimate)
				{
					const auto* masterData = DATA_MGR(data::UltimateData).GetById(0);
					const int required = masterData != nullptr ? masterData->RequiredKillCount : 0;
					const float ratio = required > 0
						? std::min(1.0f, static_cast<float>(ultimate.KillCount) / static_cast<float>(required))
						: 0.0f;

					ImGui::Text("Ultimate: %d / %d kills", ultimate.KillCount, required);
					ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f));
					if (ultimate.IsActive)
					{
						const char* phaseName = (ultimate.Phase == ecs::eUltimatePhase::Ascending) ? "Ascending" : "PlayingBeam";
						ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), "  ACTIVE (%s)", phaseName);
					}
					else if (ultimate.IsReady)
					{
						ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "  READY (Q / PadR1)");
					}
				});

			registry.view<ecs::PlayerTag, ecs::PlayerPowerChargeComponent>().each(
				[&](entt::entity playerEntity, ecs::PlayerPowerChargeComponent& charge)
				{
					const int maxCharge = ecs::ComputeMaxPowerCharge(registry, playerEntity);

					ImGui::Text("Power Charge: %d / %d", charge.Count, maxCharge);
					ImGui::SameLine();
					if (ImGui::SmallButton("+1 Charge (Debug)"))
					{
						charge.Count = std::min(maxCharge, charge.Count + 1);
					}

					if (const auto* flicker = registry.try_get<ecs::PlayerFlickerStrikeComponent>(playerEntity))
					{
						if (flicker->IsActive)
						{
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f),
								"  Flicker Strike ACTIVE (remaining hits: %d)", flicker->RemainingHits);
						}
					}
				});
		}

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Weapons", ImGuiTreeNodeFlags_DefaultOpen))
		{
			registry.view<ecs::PlayerTag, ecs::WeaponInventoryComponent>().each(
				[&](ecs::WeaponInventoryComponent& inventory)
				{
					for (entt::entity weaponEntity : inventory.Weapons)
					{
						if (!registry.valid(weaponEntity)) continue;

						auto* weapon = registry.try_get<ecs::WeaponComponent>(weaponEntity);
						if (weapon == nullptr) continue;

						const char* typeName = "Unknown";
						const char* controlName = (weapon->Control == ecs::eWeaponControl::Manual) ? "Manual" : "Auto";
						float cooldownTimer = 0.0f;
						bool hasRuntime = false;
						int orbCount = 0;
						bool hasOrbitRuntime = false;

						switch (weapon->Type)
						{
						case ecs::eWeaponType::SingleShot:
							typeName = "SingleShot";
							if (auto* rt = registry.try_get<ecs::SingleShotWeaponRuntimeComponent>(weaponEntity))
							{
								cooldownTimer = rt->CooldownTimer;
								hasRuntime = true;
							}
							break;
						case ecs::eWeaponType::AreaAttack:
							typeName = "AreaAttack";
							if (auto* rt = registry.try_get<ecs::AreaAttackWeaponRuntimeComponent>(weaponEntity))
							{
								cooldownTimer = rt->CooldownTimer;
								hasRuntime = true;
							}
							break;
						case ecs::eWeaponType::SelfDefense:
							typeName = "SelfDefense";
							if (auto* rt = registry.try_get<ecs::OrbitWeaponRuntimeComponent>(weaponEntity))
							{
								orbCount = static_cast<int>(rt->Orbs.size());
								hasOrbitRuntime = true;
							}
							break;
						case ecs::eWeaponType::Nova:
							typeName = "Nova";
							if (auto* rt = registry.try_get<ecs::NovaWeaponRuntimeComponent>(weaponEntity))
							{
								cooldownTimer = rt->CooldownTimer;
								hasRuntime = true;
							}
							break;
						case ecs::eWeaponType::Homing:
							typeName = "Homing";
							if (auto* rt = registry.try_get<ecs::HomingMissileRuntimeComponent>(weaponEntity))
							{
								cooldownTimer = rt->CooldownTimer;
								hasRuntime = true;
							}
							break;
						case ecs::eWeaponType::Chain:
							typeName = "Chain";
							if (auto* rt = registry.try_get<ecs::ChainLightningRuntimeComponent>(weaponEntity))
							{
								cooldownTimer = rt->CooldownTimer;
								hasRuntime = true;
							}
							break;
						case ecs::eWeaponType::VoidBeam:
							typeName = "VoidBeam";
							if (auto* rt = registry.try_get<ecs::VoidBeamRuntimeComponent>(weaponEntity))
							{
								cooldownTimer = rt->CooldownTimer;
								hasRuntime = true;
							}
							break;
						case ecs::eWeaponType::BoneSpear:
							typeName = "BoneSpear";
							if (auto* rt = registry.try_get<ecs::BoneSpearRuntimeComponent>(weaponEntity))
							{
								cooldownTimer = rt->CooldownTimer;
								hasRuntime = true;
							}
							break;
						case ecs::eWeaponType::Cleave:
							typeName = "Cleave";
							if (auto* rt = registry.try_get<ecs::CleaveRuntimeComponent>(weaponEntity))
							{
								cooldownTimer = rt->CooldownTimer;
								hasRuntime = true;
							}
							break;
						case ecs::eWeaponType::FlickerStrike:
							typeName = "FlickerStrike";
							if (auto* rt = registry.try_get<ecs::FlickerStrikeRuntimeComponent>(weaponEntity))
							{
								cooldownTimer = rt->CooldownTimer;
								hasRuntime = true;
							}
							break;
						default:
							break;
						}

						ImGui::PushID(static_cast<int>(weaponEntity));
						ImGui::Text("%s (Lv%d, %s, WeaponID=%d)", typeName, weapon->Level, controlName, weapon->WeaponID);

						if (hasOrbitRuntime)
						{
							// 常時稼働の武器のためクールダウンの概念が無く、周回中のオーブ数を表示する
							ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "  Orbs: %d (active)", orbCount);
						}
						else if (hasRuntime)
						{
							if (cooldownTimer <= 0.0f)
							{
								ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "  READY");
							}
							else
							{
								ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "  Cooldown: %.2fs", cooldownTimer);
							}
						}
						else
						{
							ImGui::TextDisabled("  (no runtime cooldown component)");
						}
						ImGui::PopID();
					}
				});
		}

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Enemies", ImGuiTreeNodeFlags_DefaultOpen))
		{
			int index = 0;
			registry.view<ecs::EnemyTag, ecs::EnemyStatusComponent>().each(
				[&](entt::entity entity, ecs::EnemyStatusComponent& status)
				{
					const float maxHp = status.Current.MaxHp;
					const float ratio = maxHp > 0.0f ? status.CurrentHp / maxHp : 0.0f;

					ImGui::PushID(static_cast<int>(entity));
					ImGui::Text("Enemy[%d] (Id=%d)  HP: %.1f / %.1f  EXP: %.1f",
						index, status.EnemyId, status.CurrentHp, maxHp, status.Base.ExperienceValue);
					ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f));
					ImGui::PopID();

					++index;
				});

			if (index == 0)
			{
				ImGui::TextDisabled("No enemies alive.");
			}
		}

		ImGui::End();
	}
#else
	void GameStatusDebugPanel::Draw() {}
#endif
}

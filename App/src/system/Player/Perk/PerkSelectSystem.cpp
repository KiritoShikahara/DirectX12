#include "apppch.h"
#include "PerkSelectSystem.h"

#include<Scene/Game/State/GameState.h>
#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Perk/PlayerPerkLevelComponent.h>
#include<system/Player/Level/PlayerLevelComponent.h>
#include<Scene/Game/Factory/GameSceneFactory.h>
#include<Tag/EntityTag.h>

#include<random>

namespace ecs
{
	namespace
	{
		// プロセス全体で1つの乱数エンジンを使い回す（毎フレーム再生成しない）
		std::mt19937& GetRandomEngine()
		{
			static std::mt19937 engine{ std::random_device{}() };
			return engine;
		}

		// レイアウト定数（仮想解像度1280x720基準、画面中央に3枠を横並び。個人開発プロトタイプの暫定値）
		constexpr float kOptionY = 400.0f;
		constexpr float kOptionSpacingX = 320.0f;
		constexpr float kOptionCenterX = 640.0f;
		constexpr float kOptionTextSize = 36.0f;

		const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };

		/// <summary>プレイヤーの所持武器のうち、レベルアップ可能なものが1つでもあるか</summary>
		bool HasUpgradableWeapon(entt::registry& registry)
		{
			auto playerView = registry.view<PlayerTag, WeaponInventoryComponent>();
			if (playerView.begin() == playerView.end()) return false;

			const auto& inventory = registry.get<WeaponInventoryComponent>(*playerView.begin());
			for (entt::entity weaponEntity : inventory.Weapons)
			{
				if (!registry.valid(weaponEntity)) continue;
				const auto* weapon = registry.try_get<WeaponComponent>(weaponEntity);
				if (weapon != nullptr && weapon->Level < weapon->MaxLevel)
				{
					return true;
				}
			}
			return false;
		}

		/// <summary>プレイヤーが指定の武器(種別+ID)を既に所持しているか</summary>
		bool HasWeapon(entt::registry& registry, ecs::eWeaponType type, int weaponId)
		{
			auto playerView = registry.view<PlayerTag, WeaponInventoryComponent>();
			if (playerView.begin() == playerView.end()) return false;

			const auto& inventory = registry.get<WeaponInventoryComponent>(*playerView.begin());
			for (entt::entity weaponEntity : inventory.Weapons)
			{
				if (!registry.valid(weaponEntity)) continue;
				const auto* weapon = registry.try_get<WeaponComponent>(weaponEntity);
				if (weapon != nullptr && weapon->Type == type && weapon->WeaponID == weaponId)
				{
					return true;
				}
			}
			return false;
		}

		/// <summary>プレイヤーの所持武器が上限未満で、新規武器を取得できるか</summary>
		bool HasFreeWeaponSlot(entt::registry& registry)
		{
			auto playerView = registry.view<PlayerTag, WeaponInventoryComponent>();
			if (playerView.begin() == playerView.end()) return false;

			return registry.get<WeaponInventoryComponent>(*playerView.begin()).HasFreeSlot();
		}
	}

	void PerkSelectSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto controllerView = registry.view<GameStateComponent>();
		if (controllerView.begin() == controllerView.end()) return;

		const entt::entity controllerEntity = *controllerView.begin();
		auto& gameState = registry.get<GameStateComponent>(controllerEntity);

		if (gameState.GameState != ::sys::eGameState::PerkSelect)
		{
			return;
		}

		auto* select = registry.try_get<PerkSelectComponent>(controllerEntity);
		if (select == nullptr)
		{
			// PerkSelectへ入った最初のフレーム：3択を生成しUIを表示する。
			// 入力受付は次フレームから（生成と同一フレームでの誤入力を避ける）。
			EnterPerkSelect(registry, controllerEntity);
			return;
		}

		HandleInput(registry, controllerEntity, *select);
	}

	void PerkSelectSystem::EnterPerkSelect(entt::registry& registry, entt::entity controllerEntity)
	{
		const auto& pool = GetPerkPool();
		const bool hasUpgradableWeapon = HasUpgradableWeapon(registry);
		const bool hasFreeWeaponSlot = HasFreeWeaponSlot(registry);

		// パーク種別ごとの最大レベル(data::PerkData::MaxLevel)判定用。
		// プレイヤーが未生成/コンポーネント未付与の場合は判定をスキップする(全て候補に残す)
		const std::vector<int>* pickCounts = nullptr;
		auto perkLevelView = registry.view<PlayerTag, PlayerPerkLevelComponent>();
		if (perkLevelView.begin() != perkLevelView.end())
		{
			pickCounts = &registry.get<PlayerPerkLevelComponent>(*perkLevelView.begin()).PickCounts;
		}

		// レベルアップ可能な武器が無ければ WeaponLevelUp を、
		// 空きスロットが無い/既に所持している武器なら AcquireWeapon を、
		// 種別ごとの選択回数がMaxLevelに達していれば候補から除外する
		std::vector<int> validIndices;
		validIndices.reserve(pool.size());
		for (int i = 0; i < static_cast<int>(pool.size()); ++i)
		{
			if (pool[i].Type == ePerkEffectType::WeaponLevelUp && !hasUpgradableWeapon) continue;
			if (pool[i].Type == ePerkEffectType::AcquireWeapon &&
				(!hasFreeWeaponSlot || HasWeapon(registry, pool[i].AcquireWeaponType, pool[i].AcquireWeaponId))) continue;
			if (pickCounts != nullptr && i < static_cast<int>(pickCounts->size()) &&
				(*pickCounts)[i] >= GetPerkMaxLevel(pool[i].Type)) continue;
			validIndices.push_back(i);
		}
		if (validIndices.empty()) return; // 提示できるパークが無い（現状のプールでは基本発生しない）

		std::shuffle(validIndices.begin(), validIndices.end(), GetRandomEngine());

		auto& select = registry.emplace<PerkSelectComponent>(controllerEntity);
		select.SelectedIndex = 0;

		const int choiceCount = std::min<int>(PerkSelectComponent::kChoiceCount, static_cast<int>(validIndices.size()));
		for (int i = 0; i < PerkSelectComponent::kChoiceCount; ++i)
		{
			// 候補が3件未満の場合は循環して埋める（現状のプールでは基本発生しない保険）
			select.ChoiceIndices[i] = validIndices[i % choiceCount];
		}

		auto& manager = ENTITY_MANAGER;
		for (int i = 0; i < PerkSelectComponent::kChoiceCount; ++i)
		{
			const PerkDefinition& perk = pool[select.ChoiceIndices[i]];

			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(entity);
			text.Text = perk.Name;
			text.X = kOptionCenterX + static_cast<float>(i - 1) * kOptionSpacingX - 100.0f;
			text.Y = kOptionY;
			text.Size = kOptionTextSize;
			text.Color = (i == select.SelectedIndex) ? kSelectedColor : kNormalColor;
			text.Layer = 10;

			registry.emplace<PerkOptionUiTag>(entity, i);
		}
	}

	void PerkSelectSystem::HandleInput(entt::registry& registry, entt::entity controllerEntity, PerkSelectComponent& select)
	{
		auto& input = ::sys::InputManager::Get();

		if (input.IsActionPressed("MenuLeft"))
		{
			select.SelectedIndex = (select.SelectedIndex + PerkSelectComponent::kChoiceCount - 1) % PerkSelectComponent::kChoiceCount;
		}
		else if (input.IsActionPressed("MenuRight"))
		{
			select.SelectedIndex = (select.SelectedIndex + 1) % PerkSelectComponent::kChoiceCount;
		}

		// カーソル位置に応じてハイライトを更新
		registry.view<PerkOptionUiTag, TextComponent>().each(
			[&](const PerkOptionUiTag& tag, TextComponent& text)
			{
				text.Color = (tag.OptionIndex == select.SelectedIndex) ? kSelectedColor : kNormalColor;
			});

		if (input.IsActionPressed("Select"))
		{
			const auto& pool = GetPerkPool();
			const int chosenIndex = select.ChoiceIndices[select.SelectedIndex];
			const PerkDefinition& chosen = pool[chosenIndex];
			ApplyPerk(registry, chosen);
			IncrementPerkPickCount(registry, chosenIndex);
			ExitPerkSelect(registry, controllerEntity);
		}
	}

	void PerkSelectSystem::ApplyPerk(entt::registry& registry, const PerkDefinition& perk)
	{
		auto playerView = registry.view<PlayerTag, PlayerStatusComponent>();
		if (playerView.begin() == playerView.end()) return;

		const entt::entity playerEntity = *playerView.begin();
		auto& status = registry.get<PlayerStatusComponent>(playerEntity);

		switch (perk.Type)
		{
		case ePerkEffectType::MaxHpUp:
		{
			const float beforeMaxHp = status.Current.MaxHp;
			status.Modifier.MulMaxHp += perk.Magnitude;
			status.Recompute();
			// 増加分だけ現在HPも回復する（最大HPが増えただけでは体感しにくいため）
			status.CurrentHp += (status.Current.MaxHp - beforeMaxHp);
			break;
		}
		case ePerkEffectType::CooldownDown:
			status.Modifier.MulCooldownRate += perk.Magnitude;
			status.Recompute();
			break;
		case ePerkEffectType::MoveSpeedUp:
			status.Modifier.MulMoveSpeed += perk.Magnitude;
			status.Recompute();
			break;
		case ePerkEffectType::AtkPowerUp:
			status.Modifier.MulAtkPower += perk.Magnitude;
			status.Recompute();
			break;
		case ePerkEffectType::DefenseUp:
			status.Modifier.MulDefense += perk.Magnitude;
			status.Recompute();
			break;
		case ePerkEffectType::AttackCountUp:
			status.Modifier.MulAttackCount += perk.Magnitude;
			status.Recompute();
			break;
		case ePerkEffectType::HealHp:
			status.CurrentHp = std::min(status.Current.MaxHp, status.CurrentHp + status.Current.MaxHp * perk.Magnitude);
			break;
		case ePerkEffectType::ExperienceGainUp:
		{
			auto* level = registry.try_get<PlayerLevelComponent>(playerEntity);
			if (level != nullptr)
			{
				level->MulExperienceGain += perk.Magnitude;
			}
			break;
		}
		case ePerkEffectType::WeaponLevelUp:
		{
			auto* inventory = registry.try_get<WeaponInventoryComponent>(playerEntity);
			if (inventory == nullptr) break;

			std::vector<entt::entity> upgradable;
			for (entt::entity weaponEntity : inventory->Weapons)
			{
				if (!registry.valid(weaponEntity)) continue;
				auto* weapon = registry.try_get<WeaponComponent>(weaponEntity);
				if (weapon != nullptr && weapon->Level < weapon->MaxLevel)
				{
					upgradable.push_back(weaponEntity);
				}
			}
			if (!upgradable.empty())
			{
				std::uniform_int_distribution<size_t> dist(0, upgradable.size() - 1);
				const entt::entity chosen = upgradable[dist(GetRandomEngine())];
				registry.get<WeaponComponent>(chosen).Level += 1;
			}
			break;
		}
		case ePerkEffectType::AcquireWeapon:
			// Nova/Chain Lightning/Homing Missileは全て狙い不要の自動発動想定のためAutoで追加する
			// (将来Manual操作の追加武器を用意する場合はここの制御方式を見直す)
			::ecs::GameSceneFactory::AddWeaponToPlayer(
				playerEntity, perk.AcquireWeaponType, perk.AcquireWeaponId, ::ecs::eWeaponControl::Auto);
			break;
		}
	}

	void PerkSelectSystem::IncrementPerkPickCount(entt::registry& registry, int poolIndex)
	{
		auto playerView = registry.view<PlayerTag, PlayerPerkLevelComponent>();
		if (playerView.begin() == playerView.end()) return;

		auto& pickCounts = registry.get<PlayerPerkLevelComponent>(*playerView.begin()).PickCounts;
		if (poolIndex >= 0 && poolIndex < static_cast<int>(pickCounts.size()))
		{
			pickCounts[poolIndex] += 1;
		}
	}

	void PerkSelectSystem::ExitPerkSelect(entt::registry& registry, entt::entity controllerEntity)
	{
		std::vector<entt::entity> uiEntities;
		registry.view<PerkOptionUiTag>().each(
			[&](entt::entity entity, const PerkOptionUiTag&)
			{
				uiEntities.push_back(entity);
			});
		for (entt::entity entity : uiEntities)
		{
			registry.destroy(entity);
		}

		registry.erase<PerkSelectComponent>(controllerEntity);

		auto& gameState = registry.get<GameStateComponent>(controllerEntity);
		gameState.PerkSelectDone = true;
	}
}

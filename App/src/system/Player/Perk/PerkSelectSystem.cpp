#include "apppch.h"
#include "PerkSelectSystem.h"

#include<Scene/Game/State/GameState.h>
#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Perk/PlayerPerkLevelComponent.h>
#include<system/Player/Level/PlayerLevelComponent.h>
#include<Scene/Game/Factory/GameSceneFactory.h>
#include<Scene/Game/Debug/GameDebugSettings.h>
#include<Tag/EntityTag.h>

#include<random>

namespace ecs
{
	namespace
	{
		// プロセス全体で1つの乱数エンジンを使い回す（毎フレーム再生成しない）
		std::mt19937& GetRandomEngine()
		{
			static std::mt19937 engine = ::debug::GameDebugSettings::Get().MakeRandomEngine();
			return engine;
		}

		// レイアウト定数（仮想解像度1280x720基準。5択かつ名前が長い選択肢があるため縦並び。
		// 個人開発プロトタイプの暫定値）
		constexpr float kOptionY = 240.0f;
		constexpr float kOptionSpacingY = 60.0f;
		constexpr float kOptionCenterX = 640.0f;
		constexpr float kOptionTextSize = 32.0f;

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

	int PerkSelectSystem::TakeByType(
		std::vector<int>& candidates, const std::vector<PerkDefinition>& pool, ePerkEffectType type)
	{
		// 同じ種別が複数ある(新武器は武器ごとに1件ある)ため、その中からランダムに1つ選ぶ
		std::vector<int> matched;
		for (size_t i = 0; i < candidates.size(); ++i)
		{
			if (pool[candidates[i]].Type == type) matched.push_back(static_cast<int>(i));
		}
		if (matched.empty()) return -1;

		std::uniform_int_distribution<size_t> dist(0, matched.size() - 1);
		const int pickedPos = matched[dist(GetRandomEngine())];
		const int pickedIndex = candidates[pickedPos];

		// 重複提示を避けるため、選んだものは候補から取り除く
		candidates.erase(candidates.begin() + pickedPos);
		return pickedIndex;
	}

	int PerkSelectSystem::TakeWeighted(
		std::vector<int>& candidates, const std::vector<PerkDefinition>& pool)
	{
		// 「その他」枠の抽選。1・2番目の枠で扱う武器系は対象外にする
		// (それらは枠が固定されているため、ここで重複して出す必要がない)
		float totalWeight = 0.0f;
		for (int index : candidates)
		{
			const ePerkEffectType type = pool[index].Type;
			if (type == ePerkEffectType::AcquireWeapon || type == ePerkEffectType::WeaponLevelUp) continue;
			totalWeight += GetPerkWeight(type);
		}
		if (totalWeight <= 0.0f) return -1;

		std::uniform_real_distribution<float> dist(0.0f, totalWeight);
		float threshold = dist(GetRandomEngine());

		for (size_t i = 0; i < candidates.size(); ++i)
		{
			const ePerkEffectType type = pool[candidates[i]].Type;
			if (type == ePerkEffectType::AcquireWeapon || type == ePerkEffectType::WeaponLevelUp) continue;

			threshold -= GetPerkWeight(type);
			if (threshold <= 0.0f)
			{
				const int pickedIndex = candidates[i];
				candidates.erase(candidates.begin() + i);
				return pickedIndex;
			}
		}

		return -1;
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

		auto& select = registry.emplace<PerkSelectComponent>(controllerEntity);
		select.SelectedIndex = 0;

		// ── 枠ごとの役割に沿って選択肢を決める ──────────────────────
		// 1番目: 新武器獲得(スロットが埋まっている等で出せなければ武器レベルアップ)
		// 2番目: 武器レベルアップ
		// 3番目以降: その他(PerkData::Weightによる重み付き抽選)
		// いずれも該当が無ければ「その他」で埋める。
		// 既に選んだものは除外し、同じ選択肢が重複して並ばないようにする
		std::vector<int> remaining = validIndices;

		select.ChoiceIndices[0] = TakeByType(remaining, pool, ePerkEffectType::AcquireWeapon);
		if (select.ChoiceIndices[0] < 0)
		{
			select.ChoiceIndices[0] = TakeByType(remaining, pool, ePerkEffectType::WeaponLevelUp);
		}

		select.ChoiceIndices[1] = TakeByType(remaining, pool, ePerkEffectType::WeaponLevelUp);

		for (int i = 2; i < PerkSelectComponent::kChoiceCount; ++i)
		{
			select.ChoiceIndices[i] = -1;
		}

		// 未確定の枠を「その他」から重み付きで埋める
		for (int i = 0; i < PerkSelectComponent::kChoiceCount; ++i)
		{
			if (select.ChoiceIndices[i] >= 0) continue;
			select.ChoiceIndices[i] = TakeWeighted(remaining, pool);
		}

		// それでも埋まらない場合(候補が選択肢数より少ない)は、
		// 既に提示済みのものを循環させて埋める
		int fallbackSource = -1;
		for (int i = 0; i < PerkSelectComponent::kChoiceCount; ++i)
		{
			if (select.ChoiceIndices[i] >= 0) { fallbackSource = select.ChoiceIndices[i]; break; }
		}
		if (fallbackSource < 0) return; // 1つも選べなかった(通常発生しない)

		for (int i = 0; i < PerkSelectComponent::kChoiceCount; ++i)
		{
			if (select.ChoiceIndices[i] < 0) select.ChoiceIndices[i] = fallbackSource;
		}

		auto& manager = ENTITY_MANAGER;
		for (int i = 0; i < PerkSelectComponent::kChoiceCount; ++i)
		{
			const PerkDefinition& perk = pool[select.ChoiceIndices[i]];

			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(entity);
			text.Text = perk.Name;
			// 5択かつ名前が長いもの(トレードオフ系)があるため、横並びではなく縦に並べる
			text.X = kOptionCenterX - 260.0f;
			text.Y = kOptionY + static_cast<float>(i) * kOptionSpacingY;
			text.Size = kOptionTextSize;
			text.Color = (i == select.SelectedIndex) ? kSelectedColor : kNormalColor;
			text.Layer = 10;

			registry.emplace<PerkOptionUiTag>(entity, i);
		}
	}

	void PerkSelectSystem::HandleInput(entt::registry& registry, entt::entity controllerEntity, PerkSelectComponent& select)
	{
		auto& input = ::sys::InputManager::Get();

		// 自動選択(性能計測の自動化用)。パーク選択中はTimeScale=0でゲームが停止するため、
		// 入力しない限り永久に進まず、高負荷状態を継続して計測できない。
		// 有効時は先頭の選択肢を即座に確定してゲームへ戻す
		if (::debug::GameDebugSettings::Get().IsAutoSelectPerk())
		{
			const auto& autoPool = GetPerkPool();
			const int autoIndex = select.ChoiceIndices[select.SelectedIndex];
			ApplyPerk(registry, autoPool[autoIndex]);
			IncrementPerkPickCount(registry, autoIndex);
			ExitPerkSelect(registry, controllerEntity);
			return;
		}

		// 縦並びのため上下で移動する(左右も同じ動作にして取りこぼしを防ぐ)
		if (input.IsActionPressed("MenuUp") || input.IsActionPressed("MenuLeft"))
		{
			select.SelectedIndex = (select.SelectedIndex + PerkSelectComponent::kChoiceCount - 1) % PerkSelectComponent::kChoiceCount;
		}
		else if (input.IsActionPressed("MenuDown") || input.IsActionPressed("MenuRight"))
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
		case ePerkEffectType::AllStatsUp:
		{
			// 攻撃間隔(CooldownRate)だけは「小さいほど速い」ため符号を反転して適用する
			const float beforeMaxHp = status.Current.MaxHp;
			status.Modifier.MulMaxHp += perk.Magnitude;
			status.Modifier.MulMoveSpeed += perk.Magnitude;
			status.Modifier.MulAtkPower += perk.Magnitude;
			status.Modifier.MulDefense += perk.Magnitude;
			status.Modifier.MulCooldownRate -= perk.Magnitude;
			status.Recompute();
			// MaxHpUpと同じく、増えた最大HP分は現在HPにも反映する
			status.CurrentHp += (status.Current.MaxHp - beforeMaxHp);
			break;
		}
		case ePerkEffectType::Revive:
			// 死亡時にPlayerContactDamageSystemが1つ消費して全回復させる
			status.ReviveCount += 1;
			break;
		case ePerkEffectType::GlassCannon:
		{
			status.Modifier.MulAtkPower += perk.Magnitude;
			status.Recompute();

			auto* level = registry.try_get<PlayerLevelComponent>(playerEntity);
			if (level != nullptr)
			{
				// 経験値倍率が0以下になると一切レベルアップできなくなるためクランプする
				constexpr float kMinExperienceGain = 0.1f;
				level->MulExperienceGain = std::max(
					kMinExperienceGain, level->MulExperienceGain - perk.TradeoffMagnitude);
			}
			break;
		}
		case ePerkEffectType::Berserk:
		{
			const float beforeMaxHp = status.Current.MaxHp;

			status.Modifier.MulMoveSpeed += perk.Magnitude;
			status.Modifier.MulCooldownRate -= perk.Magnitude;
			status.Modifier.MulMaxHp -= perk.TradeoffMagnitude;
			status.Recompute();

			// 最大HPが減った分は現在HPからも引く。
			// ただし0以下になると即死してしまうため、最低1は残す
			const float maxHpDelta = status.Current.MaxHp - beforeMaxHp;
			status.CurrentHp = std::max(1.0f, std::min(status.CurrentHp + maxHpDelta, status.Current.MaxHp));
			break;
		}
		case ePerkEffectType::Reckless:
			status.Modifier.MulAttackCount += perk.Magnitude;
			// 防御力は0未満にすると被ダメージ計算(半減点方式)が破綻するためクランプする
			status.Modifier.MulDefense = std::max(0.0f, status.Modifier.MulDefense - perk.TradeoffMagnitude);
			status.Recompute();
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

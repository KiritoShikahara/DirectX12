#include "apppch.h"
#include "StatusUpgradeInputSystem.h"
#include "StatusUpgradeComponent.h"

#include<Data/StatUpgrade/StatUpgradeData.h>
#include<Data/Save/PlayerSaveData.h>
#include<Scene/Hub/HubScene.h>

#include<cmath>
#include<string>
#include<algorithm>

namespace
{
	const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };
	const DirectX::XMFLOAT4 kMaxColor = { 0.5f, 0.8f, 1.0f, 1.0f };

	// フィードバックメッセージ(「ゴールドが足りません」等)の表示時間(秒)
	constexpr float kMessageDuration = 2.0f;

	// data::eStatUpgradeType(0..3)の並び順と対応する表示名。
	// CSV(StatUpgradeData::Name)は設計者向けの参考情報として残すが、UI表示はwstring
	// リテラルで直接持つ（PerkDefinition::GetPerkPool()と同じ方針。UTF-8→UTF-16の
	// 変換ユーティリティを新設せずに済む）。
	const std::wstring kOptionLabels[::ecs::StatusUpgradeComponent::kOptionCount] =
	{
		L"最大HP",
		L"攻撃力",
		L"防御力",
		L"クールダウン短縮",
		L"移動速度",
		L"ゴールド獲得量",
		L"HP自然回復",
		L"経験値獲得量",
	};

	/// <summary>PlayerSaveDataから指定インデックス(data::eStatUpgradeType)の現在レベルを取得する</summary>
	int GetLevel(const data::PlayerSaveData& save, int index)
	{
		switch (static_cast<data::eStatUpgradeType>(index))
		{
		case data::eStatUpgradeType::MaxHp:       return save.MaxHpLevel;
		case data::eStatUpgradeType::AtkPower:    return save.AtkPowerLevel;
		case data::eStatUpgradeType::Defense:     return save.DefenseLevel;
		case data::eStatUpgradeType::CooldownRate:return save.CooldownRateLevel;
		case data::eStatUpgradeType::MoveSpeed:   return save.MoveSpeedLevel;
		case data::eStatUpgradeType::GoldGainRate:return save.GoldGainRateLevel;
		case data::eStatUpgradeType::HpRegen:     return save.HpRegenLevel;
		case data::eStatUpgradeType::ExperienceGainRate: return save.ExperienceGainRateLevel;
		}
		return 0;
	}

	/// <summary>PlayerSaveDataの指定インデックス(data::eStatUpgradeType)のレベルを+1する</summary>
	void IncrementLevel(data::PlayerSaveData& save, int index)
	{
		switch (static_cast<data::eStatUpgradeType>(index))
		{
		case data::eStatUpgradeType::MaxHp:        save.MaxHpLevel += 1;       break;
		case data::eStatUpgradeType::AtkPower:     save.AtkPowerLevel += 1;    break;
		case data::eStatUpgradeType::Defense:      save.DefenseLevel += 1;     break;
		case data::eStatUpgradeType::CooldownRate: save.CooldownRateLevel += 1; break;
		case data::eStatUpgradeType::MoveSpeed:    save.MoveSpeedLevel += 1;    break;
		case data::eStatUpgradeType::GoldGainRate: save.GoldGainRateLevel += 1; break;
		case data::eStatUpgradeType::HpRegen:      save.HpRegenLevel += 1;     break;
		case data::eStatUpgradeType::ExperienceGainRate: save.ExperienceGainRateLevel += 1; break;
		}
	}

	/// <summary>PlayerSaveDataの指定インデックス(data::eStatUpgradeType)のレベルを設定する</summary>
	void SetLevel(data::PlayerSaveData& save, int index, int level)
	{
		switch (static_cast<data::eStatUpgradeType>(index))
		{
		case data::eStatUpgradeType::MaxHp:        save.MaxHpLevel = level;       break;
		case data::eStatUpgradeType::AtkPower:     save.AtkPowerLevel = level;    break;
		case data::eStatUpgradeType::Defense:      save.DefenseLevel = level;     break;
		case data::eStatUpgradeType::CooldownRate: save.CooldownRateLevel = level; break;
		case data::eStatUpgradeType::MoveSpeed:    save.MoveSpeedLevel = level;    break;
		case data::eStatUpgradeType::GoldGainRate: save.GoldGainRateLevel = level; break;
		case data::eStatUpgradeType::HpRegen:      save.HpRegenLevel = level;     break;
		case data::eStatUpgradeType::ExperienceGainRate: save.ExperienceGainRateLevel = level; break;
		}
	}

	/// <summary>1行分の表示文字列を組み立てる(レベル上限ならコストの代わりにMAXを表示)</summary>
	std::wstring BuildOptionText(int index, const data::StatUpgradeData* upgradeData, int level)
	{
		std::wstring text = kOptionLabels[index] + L"  Lv." + std::to_wstring(level);
		if (upgradeData == nullptr) return text;

		text += L"/" + std::to_wstring(upgradeData->MaxLevel);

		if (level >= upgradeData->MaxLevel)
		{
			text += L"  MAX";
		}
		else
		{
			const float cost = upgradeData->BaseCost + upgradeData->CostGrowthPerLevel * static_cast<float>(level);
			text += L"  Cost:" + std::to_wstring(static_cast<long long>(std::lround(cost))) + L"G";
		}
		return text;
	}
}

namespace ecs
{
	void StatusUpgradeInputSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto view = registry.view<StatusUpgradeComponent>();
		if (view.begin() == view.end()) return;

		auto& upgrade = view.get<StatusUpgradeComponent>(*view.begin());
		auto& input = ::sys::InputManager::Get();

		if (upgrade.MessageTimer > 0.0f)
		{
			upgrade.MessageTimer -= deltaTime;
		}

		if (upgrade.IsConfirming)
		{
			// 確認ダイアログ表示中：Select=はい/Cancel=いいえのみ受け付ける
			// （カーソル移動・HubSceneへの遷移は止める）
			if (input.IsActionPressed("Select"))
			{
				ConfirmPurchase(upgrade);
			}
			else if (input.IsActionPressed("Cancel"))
			{
				upgrade.IsConfirming = false;
				PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
			}
		}
		else
		{
			if (input.IsActionPressed("MenuUp"))
			{
				upgrade.SelectedIndex = (upgrade.SelectedIndex + StatusUpgradeComponent::kOptionCount - 1) % StatusUpgradeComponent::kOptionCount;
			}
			else if (input.IsActionPressed("MenuDown"))
			{
				upgrade.SelectedIndex = (upgrade.SelectedIndex + 1) % StatusUpgradeComponent::kOptionCount;
			}

			if (input.IsActionPressed("Select"))
			{
				TryOpenConfirm(upgrade);
			}
			// 一括最大強化(MenuRight)。1レベルずつ確認を挟むのが煩雑なため、
			// 買える範囲で最大レベルまで一気に購入する
			else if (input.IsActionPressed("MenuRight"))
			{
				PurchaseMaxLevel(upgrade);
			}
			// 全リセット(Delete)。振り直しができないと構成を試せないため、
			// 全レベルを0へ戻して消費ゴールドを全額払い戻す
			else if (input.IsActionPressed("Delete"))
			{
				ResetAllUpgrades(upgrade);
			}
			else if (input.IsActionPressed("Cancel"))
			{
				PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
				::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::HubScene>();
			}
		}

		RefreshTexts(registry, upgrade);
	}

	/// <summary>「強化しますか？」の確認ダイアログを開く。レベル上限・ゴールド不足の場合は
	/// ダイアログを開かずフィードバックメッセージを表示する</summary>
	void StatusUpgradeInputSystem::TryOpenConfirm(StatusUpgradeComponent& upgrade)
	{
		data::EnsurePlayerSaveDataLoaded();
		const auto& save = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>().Get();

		const auto* upgradeData = DATA_MGR(data::StatUpgradeData).GetById(upgrade.SelectedIndex);
		if (upgradeData == nullptr) return;

		const int level = GetLevel(save, upgrade.SelectedIndex);
		if (level >= upgradeData->MaxLevel)
		{
			ShowMessage(upgrade, L"既に最大レベルです");
			return;
		}

		const float cost = upgradeData->BaseCost + upgradeData->CostGrowthPerLevel * static_cast<float>(level);
		const int costInt = static_cast<int>(std::lround(cost));
		if (save.Gold < costInt)
		{
			ShowMessage(upgrade, L"ゴールドが足りません");
			return;
		}

		upgrade.IsConfirming = true;
		PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
	}

	/// <summary>確認ダイアログで「はい」が選ばれた時の実際の購入処理。
	/// ダイアログを開いた時点の条件(レベル上限・ゴールド)を念のため再チェックしてから確定する</summary>
	void StatusUpgradeInputSystem::ConfirmPurchase(StatusUpgradeComponent& upgrade)
	{
		upgrade.IsConfirming = false;

		data::EnsurePlayerSaveDataLoaded();
		auto& saveMgr = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>();
		auto& save = saveMgr.Get();

		const auto* upgradeData = DATA_MGR(data::StatUpgradeData).GetById(upgrade.SelectedIndex);
		if (upgradeData == nullptr) return;

		const int level = GetLevel(save, upgrade.SelectedIndex);
		if (level >= upgradeData->MaxLevel)
		{
			ShowMessage(upgrade, L"既に最大レベルです");
			return;
		}

		const float cost = upgradeData->BaseCost + upgradeData->CostGrowthPerLevel * static_cast<float>(level);
		const int costInt = static_cast<int>(std::lround(cost));
		if (save.Gold < costInt)
		{
			ShowMessage(upgrade, L"ゴールドが足りません");
			return;
		}

		save.Gold -= costInt;
		IncrementLevel(save, upgrade.SelectedIndex);
		saveMgr.Save();

		ShowMessage(upgrade, L"強化しました！");
		PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
	}

	int StatusUpgradeInputSystem::ComputeCost(const data::StatUpgradeData& upgradeData, int currentLevel)
	{
		const float cost = upgradeData.BaseCost
			+ upgradeData.CostGrowthPerLevel * static_cast<float>(currentLevel);
		return static_cast<int>(std::lround(cost));
	}

	void StatusUpgradeInputSystem::PurchaseMaxLevel(StatusUpgradeComponent& upgrade)
	{
		data::EnsurePlayerSaveDataLoaded();
		auto& saveMgr = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>();
		auto& save = saveMgr.Get();

		const auto* upgradeData = DATA_MGR(data::StatUpgradeData).GetById(upgrade.SelectedIndex);
		if (upgradeData == nullptr) return;

		int level = GetLevel(save, upgrade.SelectedIndex);
		if (level >= upgradeData->MaxLevel)
		{
			ShowMessage(upgrade, L"既に最大レベルです");
			return;
		}

		// 買える分だけ1レベルずつ購入する。
		// コストはレベルごとに変わるため、まとめて計算せず都度求める
		int purchased = 0;
		while (level < upgradeData->MaxLevel)
		{
			const int cost = ComputeCost(*upgradeData, level);
			if (save.Gold < cost) break;

			save.Gold -= cost;
			++level;
			++purchased;
		}

		if (purchased == 0)
		{
			ShowMessage(upgrade, L"ゴールドが足りません");
			return;
		}

		SetLevel(save, upgrade.SelectedIndex, level);
		saveMgr.Save();

		ShowMessage(upgrade, L"Lv." + std::to_wstring(level) + L" まで強化しました！");
		PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
	}

	void StatusUpgradeInputSystem::ResetAllUpgrades(StatusUpgradeComponent& upgrade)
	{
		data::EnsurePlayerSaveDataLoaded();
		auto& saveMgr = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>();
		auto& save = saveMgr.Get();
		auto& dataMgr = DATA_MGR(data::StatUpgradeData);

		// 全項目のレベルを0へ戻し、購入時と同じ計算式で消費ゴールドを全額払い戻す
		int refund = 0;
		bool hasAnyLevel = false;

		for (int i = 0; i < StatusUpgradeComponent::kOptionCount; ++i)
		{
			const auto* upgradeData = dataMgr.GetById(i);
			if (upgradeData == nullptr) continue;

			const int level = GetLevel(save, i);
			if (level <= 0) continue;

			hasAnyLevel = true;
			for (int l = 0; l < level; ++l)
			{
				refund += ComputeCost(*upgradeData, l);
			}
			SetLevel(save, i, 0);
		}

		if (!hasAnyLevel)
		{
			ShowMessage(upgrade, L"強化されていません");
			return;
		}

		save.Gold += refund;
		saveMgr.Save();

		ShowMessage(upgrade, std::to_wstring(refund) + L"G 払い戻しました");
		PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
	}

	void StatusUpgradeInputSystem::ShowMessage(StatusUpgradeComponent& upgrade, std::wstring message)
	{
		upgrade.Message = std::move(message);
		upgrade.MessageTimer = kMessageDuration;
	}

	void StatusUpgradeInputSystem::RefreshTexts(entt::registry& registry, const StatusUpgradeComponent& upgrade)
	{
		data::EnsurePlayerSaveDataLoaded();
		const auto& save = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>().Get();
		auto& dataMgr = DATA_MGR(data::StatUpgradeData);

		registry.view<StatusUpgradeOptionUiTag, TextComponent>().each(
			[&](const StatusUpgradeOptionUiTag& tag, TextComponent& text)
			{
				const auto* upgradeData = dataMgr.GetById(tag.OptionIndex);
				const int level = GetLevel(save, tag.OptionIndex);

				text.Text = BuildOptionText(tag.OptionIndex, upgradeData, level);

				if (upgradeData != nullptr && level >= upgradeData->MaxLevel)
				{
					text.Color = kMaxColor;
				}
				else
				{
					text.Color = (tag.OptionIndex == upgrade.SelectedIndex) ? kSelectedColor : kNormalColor;
				}
			});

		// StatusUpgradeGoldUiTag/ConfirmUiTag/MessageUiTagはデータを持たない空のタグ型のため、
		// EnTTのview.each()はこの型分の引数をコールバックへ渡さない(empty型最適化)。
		// そのためラムダはTextComponentのみを受け取る。
		registry.view<StatusUpgradeGoldUiTag, TextComponent>().each(
			[&](TextComponent& text)
			{
				text.Text = L"Gold: " + std::to_wstring(save.Gold);
			});

		registry.view<StatusUpgradeConfirmUiTag, TextComponent>().each(
			[&](TextComponent& text)
			{
				if (!upgrade.IsConfirming)
				{
					text.Text.clear();
					return;
				}

				const auto* upgradeData = dataMgr.GetById(upgrade.SelectedIndex);
				const int level = GetLevel(save, upgrade.SelectedIndex);
				const float cost = upgradeData != nullptr
					? upgradeData->BaseCost + upgradeData->CostGrowthPerLevel * static_cast<float>(level)
					: 0.0f;
				const int costInt = static_cast<int>(std::lround(cost));

				text.Text = kOptionLabels[upgrade.SelectedIndex] + L"を強化しますか？ (Cost:" +
					std::to_wstring(costInt) + L"G)\n[Select]:はい　[Cancel]:いいえ";
			});

		registry.view<StatusUpgradeMessageUiTag, TextComponent>().each(
			[&](TextComponent& text)
			{
				if (upgrade.MessageTimer > 0.0f)
				{
					text.Text = upgrade.Message;
					text.Color.w = std::clamp(upgrade.MessageTimer, 0.0f, 1.0f);
				}
				else
				{
					text.Text.clear();
				}
			});
	}
}

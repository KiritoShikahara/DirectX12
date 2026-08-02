#include "apppch.h"
#include "StatusUpgradeInputSystem.h"
#include "StatusUpgradeComponent.h"

#include<Data/StatUpgrade/StatUpgradeData.h>
#include<Data/Save/PlayerSaveData.h>
#include<Scene/Hub/HubScene.h>
#include<system/StatusUpgrade/StatusUpgradeLabels.h>
#include<system/Input/InputGuideLabels.h>
#include<graphics/Text/Renderer/TextRenderer.h>

#include<cmath>
#include<string>
#include<algorithm>

namespace
{
	const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };
	const DirectX::XMFLOAT4 kMaxColor = { 0.5f, 0.8f, 1.0f, 1.0f };

	constexpr float kMessageDuration = 2.0f;

	constexpr float kConfirmLineHeightRatio = 1.3f;
	constexpr float kConfirmDetailOffsetY = 50.0f;

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
		case data::eStatUpgradeType::AttackCount: return save.AttackCountLevel;
		case data::eStatUpgradeType::Revive:      return save.ReviveLevel;
		case data::eStatUpgradeType::PostHitInvincibility: return save.PostHitInvincibilityLevel;
		case data::eStatUpgradeType::PerkChoiceCount:      return save.PerkChoiceCountLevel;
		}
		return 0;
	}

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
		case data::eStatUpgradeType::AttackCount: save.AttackCountLevel += 1; break;
		case data::eStatUpgradeType::Revive:      save.ReviveLevel += 1;      break;
		case data::eStatUpgradeType::PostHitInvincibility: save.PostHitInvincibilityLevel += 1; break;
		case data::eStatUpgradeType::PerkChoiceCount:      save.PerkChoiceCountLevel += 1;      break;
		}
	}

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
		case data::eStatUpgradeType::AttackCount: save.AttackCountLevel = level; break;
		case data::eStatUpgradeType::Revive:      save.ReviveLevel = level;      break;
		case data::eStatUpgradeType::PostHitInvincibility: save.PostHitInvincibilityLevel = level; break;
		case data::eStatUpgradeType::PerkChoiceCount:      save.PerkChoiceCountLevel = level;      break;
		}
	}

	std::wstring BuildStateText(const data::StatUpgradeData* upgradeData, int level)
	{
		std::wstring text = L"Lv." + std::to_wstring(level);
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
			// 十字キー左右どちらでもはい/いいえをトグルする、選択肢は2つだけのため方向は問わない
			if (input.IsActionPressed("MenuLeft") || input.IsActionPressed("MenuRight"))
			{
				upgrade.ConfirmSelectedOption = (upgrade.ConfirmSelectedOption == eStatusUpgradeConfirmOption::Yes)
					? eStatusUpgradeConfirmOption::No
					: eStatusUpgradeConfirmOption::Yes;
			}

			if (input.IsActionPressed("Select"))
			{
				if (upgrade.ConfirmSelectedOption == eStatusUpgradeConfirmOption::Yes)
				{
					ConfirmMaxPurchase(upgrade);
				}
				else
				{
					upgrade.IsConfirming = false;
					PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
				}
			}
			else if (input.IsActionPressed("Cancel"))
			{
				upgrade.IsConfirming = false;
				PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
			}
		}
		else
		{
			// カーソル移動、WASD/十字キーで横4×縦2グリッドを移動する
			if (input.IsActionPressed("MenuUp"))
			{
				MoveCursor(upgrade, 0, -1);
			}
			else if (input.IsActionPressed("MenuDown"))
			{
				MoveCursor(upgrade, 0, 1);
			}
			else if (input.IsActionPressed("MenuLeft"))
			{
				MoveCursor(upgrade, -1, 0);
			}
			else if (input.IsActionPressed("MenuRight"))
			{
				MoveCursor(upgrade, 1, 0);
			}

			// Space、選択中の項目を1レベルだけ即座に強化する。確認ダイアログ無し
			if (input.IsActionPressed("Select"))
			{
				PurchaseOneLevel(upgrade);
			}
			// Enter: 「最大レベルまで強化」の確認ダイアログを開く
			else if (input.IsActionPressed("SelectAll"))
			{
				TryOpenMaxConfirm(upgrade);
			}
			// 全リセット、Delete。振り直しができないと構成を試せないため全レベルを0へ戻し消費ゴールドを全額払い戻す
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

	void StatusUpgradeInputSystem::MoveCursor(StatusUpgradeComponent& upgrade, int colDelta, int rowDelta)
	{
		// StatusUpgradeScene::CreateOptionsのkCardColumns=4と一致させること
		constexpr int kGridColumns = 4;
		constexpr int kGridRows = StatusUpgradeComponent::kOptionCount / kGridColumns;

		int col = upgrade.SelectedIndex % kGridColumns;
		int row = upgrade.SelectedIndex / kGridColumns;

		col = (col + colDelta + kGridColumns) % kGridColumns;
		row = (row + rowDelta + kGridRows) % kGridRows;

		upgrade.SelectedIndex = row * kGridColumns + col;
	}

	void StatusUpgradeInputSystem::PurchaseOneLevel(StatusUpgradeComponent& upgrade)
	{
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

		const int cost = ComputeCost(*upgradeData, level);
		if (save.Gold < cost)
		{
			ShowMessage(upgrade, L"ゴールドが足りません");
			return;
		}

		save.Gold -= cost;
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

	void StatusUpgradeInputSystem::SimulateMaxPurchase(const data::StatUpgradeData& upgradeData, int currentLevel, int gold,
		int& outTargetLevel, int& outTotalCost)
	{
		outTargetLevel = currentLevel;
		outTotalCost = 0;

		// 実際には購入しない下見用のシミュレーション。コストはレベルごとに変わるため都度求める
		while (outTargetLevel < upgradeData.MaxLevel)
		{
			const int cost = ComputeCost(upgradeData, outTargetLevel);
			if (gold - outTotalCost < cost) break;

			outTotalCost += cost;
			++outTargetLevel;
		}
	}

	void StatusUpgradeInputSystem::TryOpenMaxConfirm(StatusUpgradeComponent& upgrade)
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

		upgrade.IsConfirming = true;
		upgrade.ConfirmSelectedOption = eStatusUpgradeConfirmOption::No;
		PLAY_SE("Assets/Sound/SE/SE_Select.aud", false, 1, false);
	}

	void StatusUpgradeInputSystem::ConfirmMaxPurchase(StatusUpgradeComponent& upgrade)
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

		int targetLevel = 0, totalCost = 0;
		SimulateMaxPurchase(*upgradeData, level, save.Gold, targetLevel, totalCost);

		if (targetLevel == level)
		{
			ShowMessage(upgrade, L"ゴールドが足りません");
			return;
		}

		save.Gold -= totalCost;
		SetLevel(save, upgrade.SelectedIndex, targetLevel);
		saveMgr.Save();

		ShowMessage(upgrade, L"Lv." + std::to_wstring(targetLevel) + L" まで強化しました！");
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
		auto& textRenderer = ::graphics::TextRenderer::Get();
		const ::sys::eInputDevice device = ::sys::InputManager::Get().GetLastInputDevice();

		// 操作案内、2行。ボタン表示名は最後に使われた入力デバイスに応じて切り替える
		registry.view<StatusUpgradeGuideUiTag, TextComponent>().each(
			[&](const StatusUpgradeGuideUiTag& tag, TextComponent& text)
			{
				if (tag.LineIndex == 0)
				{
					text.Text = std::wstring(ecs::inputguide::GetMenuMoveLabel(device)) + L":選択　" +
						ecs::inputguide::GetSelectLabel(device) + L":強化　" +
						ecs::inputguide::GetSelectAllLabel(device) + L":最大まで強化";
				}
				else
				{
					text.Text = std::wstring(ecs::inputguide::GetDeleteLabel(device)) + L":全リセット(全額払い戻し)　" +
						ecs::inputguide::GetCancelLabel(device) + L":戻る";
				}
			});

		// 確認ダイアログ表示中は背後の全テキストを非表示にする。TextComponentは常にSpriteより手前に描画されるため黒背景だけでは隠せない。確認ダイアログ自身の要素だけは除外して常に表示可能にする
		registry.view<TextComponent>(entt::exclude<StatusUpgradeConfirmUiTag, StatusUpgradeConfirmOptionUiTag, StatusUpgradeConfirmGuideUiTag>).each(
			[&](TextComponent& text)
			{
				text.IsVisible = !upgrade.IsConfirming;
			});

		// カード、名前/強化状態。アイコンはSpriteでTextComponentを持たないためこのviewには自然に含まれない
		registry.view<StatusUpgradeCardUiTag, TextComponent>().each(
			[&](const StatusUpgradeCardUiTag& tag, TextComponent& text)
			{
				const auto* upgradeData = dataMgr.GetById(tag.OptionIndex);
				const int level = GetLevel(save, tag.OptionIndex);

				if (tag.Element == eStatusUpgradeCardElement::NameText)
				{
					text.Text = ecs::statusupgrade::GetOptionLabel(tag.OptionIndex);
				}
				else if (tag.Element == eStatusUpgradeCardElement::StateText)
				{
					text.Text = BuildStateText(upgradeData, level);
				}
				else
				{
					return;
				}

				// 選択中は最大レベルでも黄色を優先する。MAXを常に青にするとカンストした項目にカーソルを合わせた時にどれを選んでいるか分からなくなるため
				if (tag.OptionIndex == upgrade.SelectedIndex)
				{
					text.Color = kSelectedColor;
				}
				else if (upgradeData != nullptr && level >= upgradeData->MaxLevel)
				{
					text.Color = kMaxColor;
				}
				else
				{
					text.Color = kNormalColor;
				}

				// 水平中央揃え、WeaponIconBarSystemと同じ手法。基準はカード中心のtag.CenterX、不変
				const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
				text.X = tag.CenterX - textWidth * 0.5f;
			});

		// StatusUpgradeGoldUiTag/ConfirmUiTag/MessageUiTagは空のタグ型のため、EnTTのeachはこの型分の引数をコールバックへ渡さない
		registry.view<StatusUpgradeGoldUiTag, TextComponent>().each(
			[&](TextComponent& text)
			{
				text.Text = L"所持コイン: " + std::to_wstring(save.Gold);
			});

		// 確認ダイアログの背景ウィンドウ、黒半透明で画面中心。IsConfirming中のみ表示する
		registry.view<StatusUpgradeConfirmWindowUiTag, Sprite>().each(
			[&](Sprite& sprite)
			{
				sprite.IsVisible = upgrade.IsConfirming;
			});

		const float screenCenterX = static_cast<float>(::sys::Window::Get().GetVirtualWidth()) * 0.5f;
		const float screenCenterY = static_cast<float>(::sys::Window::Get().GetVirtualHeight()) * 0.5f;

		registry.view<StatusUpgradeConfirmUiTag, TextComponent>().each(
			[&](TextComponent& text)
			{
				if (!upgrade.IsConfirming)
				{
					text.Text.clear();
					return;
				}

				const auto* upgradeData = dataMgr.GetById(upgrade.SelectedIndex);
				if (upgradeData == nullptr)
				{
					text.Text.clear();
					return;
				}

				const int level = GetLevel(save, upgrade.SelectedIndex);

				int targetLevel = 0, totalCost = 0;
				SimulateMaxPurchase(*upgradeData, level, save.Gold, targetLevel, totalCost);

				if (targetLevel == level)
				{
					// 1レベルも買えない場合もダイアログ自体は開いたままにし、内容だけをこのメッセージに差し替える
					text.Text = L"ゴールドが足りません";
				}
				else
				{
					text.Text = std::wstring(ecs::statusupgrade::GetOptionLabel(upgrade.SelectedIndex)) +
						L"を Lv." + std::to_wstring(level) + L" → Lv." + std::to_wstring(targetLevel) + L" まで強化しますか？\n" +
						L"Gold: " + std::to_wstring(save.Gold) + L" → " + std::to_wstring(save.Gold - totalCost);
				}

				// 画面中心より上に表示し、下側にはい/いいえの選択肢と操作案内を並べる余地を残す
				const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
				text.X = screenCenterX - textWidth * 0.5f;

				const auto lineCount = std::count(text.Text.begin(), text.Text.end(), L'\n') + 1;
				const float blockHeight = static_cast<float>(lineCount) * text.Size * kConfirmLineHeightRatio;
				text.Y = screenCenterY - kConfirmDetailOffsetY - blockHeight * 0.5f;
			});

		// はい/いいえの選択肢、現在選択中の項目だけ黄色にする
		registry.view<StatusUpgradeConfirmOptionUiTag, TextComponent>().each(
			[&](const StatusUpgradeConfirmOptionUiTag& tag, TextComponent& text)
			{
				text.IsVisible = upgrade.IsConfirming;
				text.Color = (tag.Option == upgrade.ConfirmSelectedOption) ? kSelectedColor : kNormalColor;
			});

		// 操作案内、十字キーでの選択移動とSelectでの決定を案内する
		registry.view<StatusUpgradeConfirmGuideUiTag, TextComponent>().each(
			[&](TextComponent& text)
			{
				text.IsVisible = upgrade.IsConfirming;
				if (!upgrade.IsConfirming) return;

				text.Text = std::wstring(ecs::inputguide::GetMenuMoveLabel(device)) + L":選択　" +
					ecs::inputguide::GetSelectLabel(device) + L":決定";

				const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
				text.X = screenCenterX - textWidth * 0.5f;
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

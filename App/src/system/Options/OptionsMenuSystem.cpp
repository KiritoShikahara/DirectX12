#include "apppch.h"
#include "OptionsMenuSystem.h"

#include <Data/Settings/GameSettingsData.h>
#include <system/Scene/Manager/SceneManager.h>
#include <Scene/Title/TitleScene.h>

#include <algorithm>
#include <string>

namespace ecs
{
	namespace
	{
		// レイアウト(仮想解像度1280x720基準。パーク選択と同じ流儀の暫定値)
		constexpr float kTitleY = 200.0f;
		constexpr float kFirstItemY = 290.0f;
		constexpr float kItemSpacingY = 56.0f;
		constexpr float kItemX = 480.0f;
		constexpr float kTitleSize = 44.0f;
		constexpr float kItemSize = 32.0f;

		// UIレイヤーはパーク選択(10)より手前に出す
		constexpr int kUiLayer = 30;

		const DirectX::XMFLOAT4 kNormalColor = { 0.75f, 0.75f, 0.75f, 1.0f };
		const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };
		const DirectX::XMFLOAT4 kTitleColor = { 1.0f, 1.0f, 1.0f, 1.0f };

		// 十字キー/スティック1回あたりの音量変化量
		constexpr float kVolumeStep = 0.05f;

		/// <summary>音量を「MASTER  [||||||....]  60%」のような1行の文字列にする</summary>
		std::wstring MakeVolumeLabel(const wchar_t* name, float value)
		{
			constexpr int kBarLength = 10;
			const int filled = std::clamp(
				static_cast<int>(std::lround(value * kBarLength)), 0, kBarLength);

			std::wstring bar;
			bar.reserve(kBarLength);
			for (int i = 0; i < kBarLength; ++i) bar += (i < filled) ? L'|' : L'.';

			return std::wstring(name) + L"  [" + bar + L"]  "
				+ std::to_wstring(static_cast<int>(std::lround(value * 100.0f))) + L"%";
		}
	}

	int OptionsMenuSystem::GetItemCount() const
	{
		return (mPage == ePage::Root)
			? static_cast<int>(eRootItem::Count)
			: static_cast<int>(eSettingsItem::Count);
	}

	void OptionsMenuSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto& input = ::sys::InputManager::Get();

		if (!mIsOpen)
		{
			if (input.IsActionPressed("Option"))
			{
				Open(registry);
			}
			return;
		}

		HandleInput(registry);
	}

	void OptionsMenuSystem::Open(entt::registry& registry)
	{
		mIsOpen = true;
		mPage = ePage::Root;
		mSelectedIndex = 0;

		// ゲームを停止する。元の値を控えておき、閉じるときに復元する
		mPrevTimeScale = GetTime().GetTimeScale();
		GetTime().SetTimeScale(0.0);

		BuildUi();
		RefreshLabels();
	}

	void OptionsMenuSystem::Close(entt::registry& registry)
	{
		// 変更内容をここでまとめて保存する
		// (操作のたびに保存するとファイルI/Oが頻発するため)
		::data::SaveGameSettings();

		DestroyUi(registry);

		GetTime().SetTimeScale(mPrevTimeScale);
		mIsOpen = false;
	}

	void OptionsMenuSystem::ChangePage(entt::registry& registry, ePage page)
	{
		DestroyUi(registry);

		mPage = page;
		mSelectedIndex = 0;

		BuildUi();
		RefreshLabels();
	}

	void OptionsMenuSystem::DestroyUi(entt::registry& registry)
	{
		for (entt::entity entity : mUiEntities)
		{
			if (registry.valid(entity)) registry.destroy(entity);
		}
		mUiEntities.clear();
	}

	void OptionsMenuSystem::BuildUi()
	{
		auto& manager = ENTITY_MANAGER;

		// タイトル行
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(entity);
			text.Text = (mPage == ePage::Root) ? L"MENU" : L"SETTINGS";
			text.X = kItemX;
			text.Y = kTitleY;
			text.Size = kTitleSize;
			text.Color = kTitleColor;
			text.Layer = kUiLayer;
			mUiEntities.push_back(entity);
		}

		// 各項目行(中身はRefreshLabelsが埋める)
		const int itemCount = GetItemCount();
		for (int i = 0; i < itemCount; ++i)
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(entity);
			text.X = kItemX;
			text.Y = kFirstItemY + static_cast<float>(i) * kItemSpacingY;
			text.Size = kItemSize;
			text.Layer = kUiLayer;
			mUiEntities.push_back(entity);
		}
	}

	void OptionsMenuSystem::HandleInput(entt::registry& registry)
	{
		auto& input = ::sys::InputManager::Get();
		const int itemCount = GetItemCount();

		// Option / Cancel での戻る操作。
		// Settingsページからは1階層戻るだけにして、いきなり閉じないようにする
		if (input.IsActionPressed("Option") || input.IsActionPressed("Cancel"))
		{
			if (mPage == ePage::Settings)
			{
				ChangePage(registry, ePage::Root);
			}
			else
			{
				Close(registry);
			}
			return;
		}

		bool changed = false;

		if (input.IsActionPressed("MenuUp"))
		{
			mSelectedIndex = (mSelectedIndex + itemCount - 1) % itemCount;
			changed = true;
		}
		else if (input.IsActionPressed("MenuDown"))
		{
			mSelectedIndex = (mSelectedIndex + 1) % itemCount;
			changed = true;
		}
		else if (mPage == ePage::Settings && input.IsActionPressed("MenuLeft"))
		{
			AdjustSelected(-kVolumeStep);
			changed = true;
		}
		else if (mPage == ePage::Settings && input.IsActionPressed("MenuRight"))
		{
			AdjustSelected(+kVolumeStep);
			changed = true;
		}

		if (input.IsActionPressed("Select"))
		{
			Decide(registry);
			return; // Decide内でページ遷移や破棄が起きうるため、この後は触らない
		}

		if (changed) RefreshLabels();
	}

	void OptionsMenuSystem::Decide(entt::registry& registry)
	{
		if (mPage == ePage::Root)
		{
			switch (static_cast<eRootItem>(mSelectedIndex))
			{
			case eRootItem::Settings:
				ChangePage(registry, ePage::Settings);
				break;

			case eRootItem::ReturnToTitle:
				// シーン遷移前に設定を保存し、停止したTimeScaleを必ず戻す
				// (戻し忘れると遷移先が止まったままになる)
				::data::SaveGameSettings();
				DestroyUi(registry);
				GetTime().SetTimeScale(1.0);
				mIsOpen = false;
				::sys::SceneManager::Get().ChangeSceneWithTransition<::scene::TitleScene>();
				break;

			case eRootItem::Resume:
				Close(registry);
				break;

			default:
				break;
			}
			return;
		}

		// Settingsページ
		if (static_cast<eSettingsItem>(mSelectedIndex) == eSettingsItem::Back)
		{
			ChangePage(registry, ePage::Root);
		}
	}

	void OptionsMenuSystem::AdjustSelected(float delta)
	{
		auto& configReg = ::data::ConfigRegistry::Get();
		if (!configReg.IsRegistered<::data::GameSettingsData>()) return;

		auto& manager = configReg.GetManager<::data::GameSettingsData>();
		auto& settings = manager.Get();

		float* target = nullptr;
		switch (static_cast<eSettingsItem>(mSelectedIndex))
		{
		case eSettingsItem::MasterVolume: target = &settings.MasterVolume; break;
		case eSettingsItem::BgmVolume:    target = &settings.BgmVolume;    break;
		case eSettingsItem::SeVolume:     target = &settings.SeVolume;     break;
		default: return; // Backなど値を持たない項目
		}

		*target = std::clamp(*target + delta, 0.0f, 1.0f);

		// 変更をConfigManagerへ通知し、音量を即座に反映する
		// (保存はメニューを閉じたときにまとめて行う)
		manager.NotifyChanged();
		::data::ApplyGameSettings();
	}

	void OptionsMenuSystem::RefreshLabels()
	{
		auto& registry = ENTITY_MANAGER.GetRegistry();
		const int itemCount = GetItemCount();

		const ::data::GameSettingsData* settings = nullptr;
		auto& configReg = ::data::ConfigRegistry::Get();
		if (configReg.IsRegistered<::data::GameSettingsData>())
		{
			settings = &configReg.GetManager<::data::GameSettingsData>().Get();
		}

		for (int i = 0; i < itemCount; ++i)
		{
			// mUiEntities[0]はタイトル行のため、項目はi+1番目に対応する
			const size_t uiIndex = static_cast<size_t>(i) + 1;
			if (uiIndex >= mUiEntities.size()) break;

			auto* text = registry.try_get<TextComponent>(mUiEntities[uiIndex]);
			if (text == nullptr) continue;

			if (mPage == ePage::Root)
			{
				switch (static_cast<eRootItem>(i))
				{
				case eRootItem::Settings:      text->Text = L"設定"; break;
				case eRootItem::ReturnToTitle: text->Text = L"タイトルへ"; break;
				case eRootItem::Resume:        text->Text = L"戻る"; break;
				default: break;
				}
			}
			else if (settings != nullptr)
			{
				switch (static_cast<eSettingsItem>(i))
				{
				case eSettingsItem::MasterVolume: text->Text = MakeVolumeLabel(L"MASTER", settings->MasterVolume); break;
				case eSettingsItem::BgmVolume:    text->Text = MakeVolumeLabel(L"BGM   ", settings->BgmVolume);    break;
				case eSettingsItem::SeVolume:     text->Text = MakeVolumeLabel(L"SE    ", settings->SeVolume);     break;
				case eSettingsItem::Back:         text->Text = L"戻る"; break;
				default: break;
				}
			}

			text->Color = (i == mSelectedIndex) ? kSelectedColor : kNormalColor;
		}
	}
}

#include "apppch.h"
#include "OptionsMenuSystem.h"

#include <Data/Settings/GameSettingsData.h>
#include <system/Scene/Manager/SceneManager.h>
#include <system/Input/InputGuideLabels.h>
#include <system/UI/UiPanelUtility.h>
#include <graphics/Text/Renderer/TextRenderer.h>
#include <Scene/Title/TitleScene.h>

#include <algorithm>
#include <limits>
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
		switch (mPage)
		{
		case ePage::Root:     return static_cast<int>(eRootItem::Count);
		case ePage::Settings: return static_cast<int>(eSettingsItem::Count);
		case ePage::Controls: return 1; // 選択可能な項目は「戻る」のみ(一覧は選択不可の固定表示)
		}
		return 0;
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
		BuildBackgroundPanel();
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
		BuildBackgroundPanel();
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
			if (mPage == ePage::Root) text.Text = L"MENU";
			else if (mPage == ePage::Controls) text.Text = L"CONTROLS";
			else text.Text = L"SETTINGS";
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

		if (mPage == ePage::Controls)
		{
			BuildControlsInfoLines();
		}
	}

	void OptionsMenuSystem::BuildControlsInfoLines()
	{
		// 選択項目("戻る"、itemCount=1件)より下に、選択不可の操作方法一覧を並べる。
		// mUiEntitiesの末尾に追加するだけなのでRefreshLabels(0..itemCount-1しか触らない)の
		// 対象外になり、DestroyUiでは他のUIエンティティと同様にまとめて破棄される。
		auto& manager = ENTITY_MANAGER;
		const ::sys::eInputDevice device = ::sys::InputManager::Get().GetLastInputDevice();

		const std::wstring lines[] =
		{
			L"移動: " + std::wstring(::ecs::inputguide::GetGameplayMoveLabel(device)),
			L"照準: " + std::wstring(::ecs::inputguide::GetAimLabel(device)),
			L"メイン攻撃: " + std::wstring(::ecs::inputguide::GetAttackLabel(device)),
			L"サブ攻撃: " + std::wstring(::ecs::inputguide::GetAttack2Label(device)),
			L"必殺技: " + std::wstring(::ecs::inputguide::GetUltimateLabel(device)),
			L"フリッカーストライク: " + std::wstring(::ecs::inputguide::GetFlickerStrikeLabel(device)),
			L"ポーズメニュー: " + std::wstring(::ecs::inputguide::GetOptionLabel(device)),
		};

		constexpr float kInfoStartY = kFirstItemY + kItemSpacingY; // 「戻る」(1件)の次の行から
		constexpr float kInfoSize = 28.0f;
		const DirectX::XMFLOAT4 kInfoColor = { 0.85f, 0.85f, 0.85f, 1.0f };

		for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i)
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(entity);
			text.Text = lines[i];
			text.X = kItemX;
			text.Y = kInfoStartY + static_cast<float>(i) * kItemSpacingY;
			text.Size = kInfoSize;
			text.Color = kInfoColor;
			text.Layer = kUiLayer;
			mUiEntities.push_back(entity);
		}
	}

	void OptionsMenuSystem::BuildBackgroundPanel()
	{
		// ゲーム画面(一時停止中の背景)の上に文字が直接乗ると読みづらいため、黒半透明の板を
		// メニュー全体の下に敷く。ページごとに行数・文字幅が変わる(Controlsページは操作方法
		// 一覧の分だけ縦に長い)ため、生成済みテキストの実測範囲から動的にサイズを決める
		// (呼び出しはRefreshLabelsで文字列が確定した後であること)。
		auto& registry = ENTITY_MANAGER.GetRegistry();
		auto& textRenderer = ::graphics::TextRenderer::Get();

		float minX = (std::numeric_limits<float>::max)();
		float maxX = (std::numeric_limits<float>::lowest)();
		float minY = (std::numeric_limits<float>::max)();
		float maxY = (std::numeric_limits<float>::lowest)();

		for (entt::entity entity : mUiEntities)
		{
			const auto* text = registry.try_get<TextComponent>(entity);
			if (text == nullptr) continue;

			minX = std::min(minX, text->X);
			maxX = std::max(maxX, text->X + textRenderer.MeasureWidth(text->Text, text->Size));
			minY = std::min(minY, text->Y);
			maxY = std::max(maxY, text->Y + text->Size);
		}

		if (minX > maxX) return; // テキストが1件も無い場合の保険(理論上起きない)

		constexpr float kPanelPadX = 60.0f;
		constexpr float kPanelPadTop = 30.0f;
		constexpr float kPanelPadBottom = 30.0f;

		const float left = minX - kPanelPadX;
		const float right = maxX + kPanelPadX;
		const float top = minY - kPanelPadTop;
		const float bottom = maxY + kPanelPadBottom;

		auto panelEntity = ::ecs::uiutil::CreateTranslucentPanel(
			(left + right) * 0.5f, (top + bottom) * 0.5f,
			right - left, bottom - top,
			0);
		mUiEntities.push_back(panelEntity);
	}

	void OptionsMenuSystem::HandleInput(entt::registry& registry)
	{
		auto& input = ::sys::InputManager::Get();
		const int itemCount = GetItemCount();

		// Option / Cancel での戻る操作。
		// Root以外のページからは1階層戻るだけにして、いきなり閉じないようにする
		if (input.IsActionPressed("Option") || input.IsActionPressed("Cancel"))
		{
			if (mPage != ePage::Root)
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

			case eRootItem::Controls:
				ChangePage(registry, ePage::Controls);
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

		// Controlsページ: 選択可能な項目は「戻る」の1件のみ
		if (mPage == ePage::Controls)
		{
			ChangePage(registry, ePage::Root);
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
				case eRootItem::Controls:      text->Text = L"操作案内"; break;
				case eRootItem::ReturnToTitle: text->Text = L"タイトルへ"; break;
				case eRootItem::Resume:        text->Text = L"戻る"; break;
				default: break;
				}
			}
			else if (mPage == ePage::Controls)
			{
				// 選択可能な項目は「戻る」の1件のみ(操作方法の一覧はBuildControlsInfoLinesが
				// 選択対象外の固定表示として別途生成している)
				if (i == 0) text->Text = L"戻る";
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

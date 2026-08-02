#include "apppch.h"
#include "PerkSelectSystem.h"

#include<Scene/Game/State/GameState.h>
#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Perk/PlayerPerkLevelComponent.h>
#include<system/Player/Level/PlayerLevelComponent.h>
#include<Scene/Game/Factory/GameSceneFactory.h>
#include<Scene/Game/Debug/GameDebugSettings.h>
#include<Data/Save/PlayerSaveData.h>
#include<Tag/EntityTag.h>
#include<graphics/Text/Renderer/TextRenderer.h>
#include<system/Window/Window.h>
#include<system/Player/Weapon/WeaponIconRegistry.h>
#include<system/UI/UiPanelUtility.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Input/InputGuideLabels.h>

#include<algorithm>
#include<random>

namespace ecs
{
	namespace
	{
		std::mt19937& GetRandomEngine()
		{
			// プロセス全体で使い回す
			static std::mt19937 engine = ::debug::GameDebugSettings::Get().MakeRandomEngine();
			return engine;
		}

		constexpr float kOptionTextSize = 32.0f;

		const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };

		constexpr float kCardSize = 180.0f;
		constexpr float kCardSpacingX = 360.0f;
		constexpr float kIconTextGapY = 40.0f;

		constexpr float kWindowPadX = 200.0f;
		constexpr float kWindowPadY = 170.0f;

		constexpr float kBackgroundAlpha = 0.12f;

		constexpr const char* kPerkConfirmEffectPath = "Assets/Effect/Herald.efk";

		const char* GetPerkIconPath(const PerkDefinition& perk)
		{
			// 未作成アイコンは代用アイコンで済ませ、後で差し替える
			constexpr const char* kFallbackIcon = "Assets/Icon/loading.png";

			switch (perk.Type)
			{
			case ePerkEffectType::MaxHpUp:          return "Assets/Icon/health.png";
			case ePerkEffectType::AtkPowerUp:       return "Assets/Icon/attack_up.png";
			case ePerkEffectType::DefenseUp:        return "Assets/Icon/defense.png";
			case ePerkEffectType::HealHp:           return "Assets/Icon/heel.png";
			case ePerkEffectType::Revive:           return "Assets/Icon/revive.png";
			case ePerkEffectType::GlassCannon:      return "Assets/Icon/fire.png";      // 攻撃特化の暫定割当
			case ePerkEffectType::MoveSpeedUp:      return "Assets/Icon/speed_up.png";
			case ePerkEffectType::ExperienceGainUp: return "Assets/Icon/exp_up.png";
			case ePerkEffectType::AllStatsUp:       return "Assets/Icon/all_status.png";

			case ePerkEffectType::AttackCountUp:
			case ePerkEffectType::Reckless:    return "Assets/Icon/attack_up.png"; // 攻撃回数系の暫定割当

			case ePerkEffectType::AcquireWeapon:
				// 所持武器バーと同じ対応表を使い、選択画面とバーでアイコンが食い違わないようにする
				return ecs::weaponutil::GetWeaponIconPath(perk.AcquireWeaponType);

			default:
				// CooldownDown/WeaponLevelUp/Berserk は未作成
				return kFallbackIcon;
			}
		}

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
			// PerkSelectへ入った最初のフレームで3択を生成しUIを表示する。入力受付は次フレームから
			EnterPerkSelect(registry, controllerEntity);
			return;
		}

		// 設定メニューが開いている間はパーク選択の入力を止める。無いとメニューを開いた直後の入力がパーク選択としても二重処理されてしまう
		if (gameState.IsOptionsMenuOpen)
		{
			return;
		}

		HandleInput(registry, controllerEntity, *select);
	}

	int PerkSelectSystem::TakeByType(
		std::vector<int>& candidates, const std::vector<PerkDefinition>& pool, ePerkEffectType type)
	{
		// 同じ種別が複数ある、新武器は武器ごとに1件あるため、その中からランダムに1つ選ぶ
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
		// 新武器獲得・武器レベルアップも含めた全種別からPerkData::Weightの重み付きで抽選する
		float totalWeight = 0.0f;
		for (int index : candidates)
		{
			totalWeight += GetPerkWeight(pool[index].Type);
		}
		if (totalWeight <= 0.0f) return -1;

		std::uniform_real_distribution<float> dist(0.0f, totalWeight);
		float threshold = dist(GetRandomEngine());

		for (size_t i = 0; i < candidates.size(); ++i)
		{
			threshold -= GetPerkWeight(pool[candidates[i]].Type);
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

		// パーク種別ごとの最大レベル判定用。プレイヤー未生成の場合は判定をスキップする
		const std::vector<int>* pickCounts = nullptr;
		auto perkLevelView = registry.view<PlayerTag, PlayerPerkLevelComponent>();
		if (perkLevelView.begin() != perkLevelView.end())
		{
			pickCounts = &registry.get<PlayerPerkLevelComponent>(*perkLevelView.begin()).PickCounts;
		}

		// レベルアップ可能な武器が無ければWeaponLevelUpを、空きスロットが無い/既に所持している武器ならAcquireWeaponを、選択回数がMaxLevelに達していれば候補から除外する
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
		if (validIndices.empty()) return; // 提示できるパークが無い、現状のプールでは基本発生しない

		auto& select = registry.emplace<PerkSelectComponent>(controllerEntity);
		select.SelectedIndex = 0;

		// ショップ強化パーク選択肢+1を取得していれば4択、未取得ならデフォルトの3択にする
		data::EnsurePlayerSaveDataLoaded();
		const auto& save = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>().Get();
		select.ChoiceCount = (save.PerkChoiceCountLevel > 0)
			? PerkSelectComponent::kMaxChoiceCount
			: PerkSelectComponent::kDefaultChoiceCount;

		// 枠ごとの役割に沿って選択肢を決める。1番目は新武器獲得か武器レベルアップかを半々のランダムで決め、無ければもう一方にフォールバックする。
		// 2番目以降は全種別をPerkData::Weightで重み付き抽選する。選んだものは候補から除外し重複を防ぐ
		std::vector<int> remaining = validIndices;

		const bool preferAcquire = std::uniform_int_distribution<int>(0, 1)(GetRandomEngine()) == 0;
		const ePerkEffectType firstChoiceType = preferAcquire ? ePerkEffectType::AcquireWeapon : ePerkEffectType::WeaponLevelUp;
		const ePerkEffectType firstChoiceFallbackType = preferAcquire ? ePerkEffectType::WeaponLevelUp : ePerkEffectType::AcquireWeapon;

		select.ChoiceIndices[0] = TakeByType(remaining, pool, firstChoiceType);
		if (select.ChoiceIndices[0] < 0)
		{
			select.ChoiceIndices[0] = TakeByType(remaining, pool, firstChoiceFallbackType);
		}

		for (int i = 1; i < select.ChoiceCount; ++i)
		{
			select.ChoiceIndices[i] = TakeWeighted(remaining, pool);
		}

		// それでも埋まらない場合は、既に提示済みのものを循環させて埋める
		int fallbackSource = -1;
		for (int i = 0; i < select.ChoiceCount; ++i)
		{
			if (select.ChoiceIndices[i] >= 0) { fallbackSource = select.ChoiceIndices[i]; break; }
		}
		if (fallbackSource < 0) return; // 1つも選べなかった、通常発生しない

		for (int i = 0; i < select.ChoiceCount; ++i)
		{
			if (select.ChoiceIndices[i] < 0) select.ChoiceIndices[i] = fallbackSource;
		}

		// 画面中心を基準に選択肢カードを横へ均等配置する。中央インデックスのカード中心を画面中心に一致させる
		const float screenCenterX = static_cast<float>(::sys::Window::Get().GetVirtualWidth()) * 0.5f;
		const float screenCenterY = static_cast<float>(::sys::Window::Get().GetVirtualHeight()) * 0.5f;

		auto& manager = ENTITY_MANAGER;
		auto& textRenderer = ::graphics::TextRenderer::Get();

		// アイコン画像の中心は画面中心の高さに、テキストはその下、アイコン下端+余白に置く
		const float iconCenterY = screenCenterY;
		const float textBaselineY = iconCenterY + kCardSize * 0.5f + kIconTextGapY;

		// 選択肢名の最大行数を求めテキスト表示領域の高さに反映する
		int maxOptionTextLines = 1;
		for (int i = 0; i < select.ChoiceCount; ++i)
		{
			const std::wstring& name = pool[select.ChoiceIndices[i]].Name;
			const int lineCount = static_cast<int>(std::count(name.begin(), name.end(), L'\n')) + 1;
			maxOptionTextLines = std::max(maxOptionTextLines, lineCount);
		}
		const float optionTextBlockHeight =
			kOptionTextSize + static_cast<float>(maxOptionTextLines - 1) * textRenderer.MeasureLineHeight(kOptionTextSize);

		// 背景、タイトル画面の背景を流用。パーク選択中は背後の3Dシーンの代わりにこの背景を見せる
		{
			auto bgEntity = manager.CreateEntity();
			auto& bgTransform = manager.AddComponent<ecs::Transform>(bgEntity);
			bgTransform.Set2DPosition(0.0f, 0.0f); // 左上原点、Pivot既定0,0で全画面を覆う

			// TitleSceneと同じパス、Windowsは大小無視で実ファイルへ解決される
			auto bgTexture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");
			auto& bgSprite = manager.AddComponent<ecs::Sprite>(bgEntity, bgTexture);
			bgSprite.Size = { screenCenterX * 2.0f, screenCenterY * 2.0f };            // 仮想解像度全体
			bgSprite.Color = ::graphics::Color(1.0f, 1.0f, 1.0f, kBackgroundAlpha);    // ほぼ透明、うっすら見える程度
			bgSprite.SetLayer(::ecs::SpriteLayer::UI, -1);                             // 黒ウィンドウのoffset0より奥

			// ExitPerkSelectでまとめて破棄させる。TextComponentを持たないためハイライト対象外。
			registry.emplace<PerkOptionUiTag>(bgEntity, -1);
		}

		// 操作説明はカードのテキスト下端から間隔を空けて置き、ウィンドウはこの行を内側に収めるところまで下へ拡張する
		constexpr float kHelpTextSize = 24.0f;
		constexpr float kHelpGapY = 36.0f;       // カードのテキスト下端から操作説明までの間隔、px
		constexpr float kHelpBottomPadY = 50.0f; // 操作説明の下端からウィンドウ下端までの余白、px
		const float helpTextY = textBaselineY + optionTextBlockHeight + kHelpGapY;

		// 選択肢全体を囲む黒半透明ウィンドウ、アイコンより奥。横は左右端カードの外側までpad、縦はアイコン上端から操作説明下端+padまでを覆う
		const float windowTop = iconCenterY - kCardSize * 0.5f - kWindowPadY;
		const float windowBottom = helpTextY + kHelpTextSize + kHelpBottomPadY;
		{
			const float windowWidth =
				kCardSpacingX * static_cast<float>(select.ChoiceCount - 1)
				+ kCardSize + kWindowPadX * 2.0f;

			auto windowEntity = ::ecs::uiutil::CreateTranslucentPanel(
				screenCenterX, (windowTop + windowBottom) * 0.5f,
				windowWidth, windowBottom - windowTop,
				0); // アイコンのoffset4より奥、小さいLayer

			// ExitPerkSelectでまとめて破棄させる。TextComponentを持たないためハイライト対象外。
			registry.emplace<PerkOptionUiTag>(windowEntity, -1);
		}

		for (int i = 0; i < select.ChoiceCount; ++i)
		{
			const PerkDefinition& perk = pool[select.ChoiceIndices[i]];

			// カード中心X。iを中央からのオフセットに変換して横に均等配置する、i=1が画面中心
			const float indexFromCenter =
				static_cast<float>(i) - static_cast<float>(select.ChoiceCount - 1) * 0.5f;
			const float cardCenterX = screenCenterX + indexFromCenter * kCardSpacingX;

			// 大きめのアイコン画像、中心は画面中心の高さ。未作成アイコンはGetPerkIconPathが代用画像を返す
			auto iconEntity = manager.CreateEntity();
			auto& iconTransform = manager.AddComponent<ecs::Transform>(iconEntity);
			iconTransform.Set2DPosition(cardCenterX, iconCenterY);

			auto iconTexture = ::graphics::TextureManager::Get().GetOrLoad(GetPerkIconPath(perk));
			auto& iconSprite = manager.AddComponent<ecs::Sprite>(iconEntity, iconTexture);
			iconSprite.Pivot = { 0.5f, 0.5f };              // Set2DPositionの座標を画像の中心に合わせる
			iconSprite.Size = { kCardSize, kCardSize };     // 元画像サイズに依らず一定の大きさで表示
			iconSprite.SetLayer(::ecs::SpriteLayer::UI, 4); // ウィンドウのoffset0より手前

			registry.emplace<PerkOptionUiTag>(iconEntity, i);

			// テキストはアイコンの下、cardCenterXへ水平中央揃え。別パスで描画されるためアイコン・ウィンドウより常に前面に出る
			const float textWidth = textRenderer.MeasureWidth(perk.Name, kOptionTextSize);

			auto textEntity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(textEntity);
			text.Text = perk.Name;
			text.X = cardCenterX - textWidth * 0.5f;
			text.Y = textBaselineY;
			text.Size = kOptionTextSize;
			text.Color = (i == select.SelectedIndex) ? kSelectedColor : kNormalColor;
			text.Layer = 10;

			// 画像・テキストとも同じタグを付けExitPerkSelectでまとめて破棄する。ハイライト処理はTextComponentを持つものだけを見るためアイコンには影響しない
			registry.emplace<PerkOptionUiTag>(textEntity, i);
		}

		// 操作説明、黒ウィンドウ内で選択肢カードの下中央。選択肢とは別に常時固定表示し、最後に使われた入力デバイスに応じてボタン名を出し分ける
		{
			const DirectX::XMFLOAT4 kHelpColor = { 0.85f, 0.85f, 0.85f, 1.0f };

			const ::sys::eInputDevice device = ::sys::InputManager::Get().GetLastInputDevice();
			const std::wstring helpText =
				std::wstring(::ecs::inputguide::GetMenuMoveLabel(device)) + L" : 選択　" +
				std::wstring(::ecs::inputguide::GetSelectLabel(device)) + L" : 決定";

			const auto entities = ::ecs::uiutil::CreateTextLines(
				{ helpText }, ::ecs::uiutil::eTextHorizontalAlign::Center,
				screenCenterX, helpTextY, 0.0f, kHelpTextSize, kHelpColor, 10);

			// 選択肢ではないため-1タグ、ExitPerkSelectでまとめて破棄、ハイライト対象外
			registry.emplace<PerkOptionUiTag>(entities[0], -1);
		}
	}

	void PerkSelectSystem::HandleInput(entt::registry& registry, entt::entity controllerEntity, PerkSelectComponent& select)
	{
		auto& input = ::sys::InputManager::Get();

		// 自動選択、性能計測の自動化用。パーク選択中はTimeScale=0で停止するため入力が無いと永久に進まない。有効時は先頭の選択肢を即座に確定する
		if (::debug::GameDebugSettings::Get().IsAutoSelectPerk())
		{
			const auto& autoPool = GetPerkPool();
			const int autoIndex = select.ChoiceIndices[select.SelectedIndex];
			ApplyPerk(registry, autoPool[autoIndex]);
			IncrementPerkPickCount(registry, autoIndex);
			ExitPerkSelect(registry, controllerEntity);
			return;
		}

		// 縦並びのため上下で移動する、左右も同じ動作にして取りこぼしを防ぐ
		if (input.IsActionPressed("MenuUp") || input.IsActionPressed("MenuLeft"))
		{
			select.SelectedIndex = (select.SelectedIndex + select.ChoiceCount - 1) % select.ChoiceCount;
		}
		else if (input.IsActionPressed("MenuDown") || input.IsActionPressed("MenuRight"))
		{
			select.SelectedIndex = (select.SelectedIndex + 1) % select.ChoiceCount;
		}

		// カーソル位置に応じてハイライトを更新
		registry.view<PerkOptionUiTag, TextComponent>().each(
			[&](const PerkOptionUiTag& tag, TextComponent& text)
			{
				text.Color = (tag.OptionIndex == select.SelectedIndex) ? kSelectedColor : kNormalColor;
			});

		// マウスクリックでの誤確定を防ぐため、Selectのマウス割り当ては無視する
		if (input.IsActionPressedExcludingMouse("Select"))
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

		// パーク確定時の演出。低頻度イベントのため常時ヒットするエフェクトより多少リッチなものを使う
		if (const auto* transform = registry.try_get<Transform>(playerEntity))
		{
			ecs::effectutil::PlayOneShotCombined(kPerkConfirmEffectPath, transform->GetPosition(), 1.0f);
		}

		switch (perk.Type)
		{
		case ePerkEffectType::MaxHpUp:
		{
			const float beforeMaxHp = status.Current.MaxHp;
			status.Modifier.MulMaxHp += perk.Magnitude;
			status.Recompute();
			// 増加分だけ現在HPも回復する、最大HPが増えただけでは体感しにくいため
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
			// 攻撃間隔のCooldownRateだけは小さいほど速いため符号を反転して適用する
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

			// 最大HPが減った分は現在HPからも引く。ただし0以下になると即死するため最低1は残す
			const float maxHpDelta = status.Current.MaxHp - beforeMaxHp;
			status.CurrentHp = std::max(1.0f, std::min(status.CurrentHp + maxHpDelta, status.Current.MaxHp));
			break;
		}
		case ePerkEffectType::Reckless:
			status.Modifier.MulAttackCount += perk.Magnitude;
			// 防御力は0未満にすると被ダメージ計算の半減点方式が破綻するためクランプする
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

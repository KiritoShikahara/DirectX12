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
#include<graphics/Text/Renderer/TextRenderer.h> // テキスト幅を測って画像中央へ揃えるため
#include<system/Window/Window.h>                // 画面中心(仮想解像度)を基準に配置するため
#include<system/Player/Weapon/WeaponIconRegistry.h> // 武器種別→アイコンの対応表(WeaponIconBarSystemと共通)
#include<system/UI/UiPanelUtility.h>
#include<system/Effect/EffectSpawnUtility.h>
#include<system/Input/InputGuideLabels.h> // 操作説明の文言(デバイスに応じたボタン名)

#include<algorithm>
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

		// レイアウト定数。テキスト/スプライトとも同一座標系(ウィンドウ仮想解像度・左上原点)で配置する。
		// 選択肢は「大きめのアイコン画像の上にテキストを重ねたカード」を、画面中心を基準に横へ均等配置する
		// (2つ目のカード中心が画面中心に一致)。メリット+デメリット系のトレードオフパークは
		// PerkDefinition側で"\n"を挟んで2行にしてあり(横幅溢れ対策)、EnterPerkSelectが
		// 実際の行数を見てカード表示領域の高さを動的に広げる。個人開発プロトタイプの暫定値。
		constexpr float kOptionTextSize = 32.0f; // 選択肢テキストの文字高さ(px)

		const DirectX::XMFLOAT4 kNormalColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		const DirectX::XMFLOAT4 kSelectedColor = { 1.0f, 0.9f, 0.2f, 1.0f };

		// カード(アイコン画像 + その上に重ねるテキスト)のレイアウト。
		constexpr float kCardSize = 180.0f;      // アイコン画像の表示サイズ(px、正方形)。元画像サイズに依らず一定
		constexpr float kCardSpacingX = 360.0f;  // カード中心どうしの横間隔(px)。3つを横に均等配置するのに使う
		constexpr float kIconTextGapY = 40.0f;   // アイコン画像の下端からテキスト(ベースライン)までの間隔(px)

		// 選択肢全体を囲む黒半透明ウィンドウ(視認性向上用)のパラメータ。
		// アイコンより奥へ敷く必要があるが、Shapeは描画順がSprite→Shape→Textでアイコン(Sprite)の
		// 上に来てしまうため使えない。UiPanelUtility(白テクスチャをColorで黒+半透明に着色した
		// Spriteで代用)で敷く。描画は「Layerが大きいほど前面」(painter順。SpriteRenderer参照)
		// なので、ウィンドウは小さいLayer(奥)、アイコンは大きいLayer(手前)に置く。テキストは別パスで最前面。
		constexpr float kWindowPadX = 200.0f;    // ウィンドウ左右の余白(px)。大きいほどウィンドウが横に広がる
		constexpr float kWindowPadY = 170.0f;    // ウィンドウ上下の余白(px)。大きいほどウィンドウが縦に広がる

		// 背景(タイトル画像)の不透明度。ほとんど見えない程度のうっすらした背景にする(0=透明,1=不透明)
		constexpr float kBackgroundAlpha = 0.12f;

		// パーク確定演出。低頻度イベントのため常時ヒットするエフェクトより多少リッチなものを使う
		constexpr const char* kPerkConfirmEffectPath = "Assets/Effect/Herald.efk";

		/// <summary>
		/// パーク1件に対応する表示アイコンのパスを返す。
		/// 専用アイコンが未作成のパークは代用アイコン(kFallbackIcon)を返す
		/// (あとで専用アイコンを Assets/Icon に追加したら、ここの割り当てを差し替えるだけでよい)。
		/// </summary>
		const char* GetPerkIconPath(const PerkDefinition& perk)
		{
			// 未作成アイコンの代用(後で差し替える前提のプレースホルダ)
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
				// 所持武器バー(WeaponIconBarSystem)と同じ対応表を使い、選択画面とバーで
				// アイコンが食い違わないようにする(WeaponIconRegistry参照)。
				return ecs::weaponutil::GetWeaponIconPath(perk.AcquireWeaponType);

			default:
				// CooldownDown/WeaponLevelUp/Berserk は未作成
				return kFallbackIcon;
			}
		}

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

		// 設定メニュー(OptionsMenuSystem)が開いている間はパーク選択の入力を止める。
		// Escapeキーは"Option"と"Cancel"の両方に割り当てられているため、これが無いと
		// メニューを開いた直後にMenuUp/Down/Selectがパーク選択のカーソル移動・確定としても
		// 二重に処理されてしまう(パーク選択が意図せず同時に進んでしまう問題への対処)。
		if (gameState.IsOptionsMenuOpen)
		{
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
		// 新武器獲得・武器レベルアップも含めた全種別からPerkData::Weightの重み付きで抽選する
		// (以前は1・2番目の枠に武器系を固定で割り当てていたため対象外にしていたが、
		// 全ての枠を等しく抽選するよう統合した)
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

		// ショップ強化「パーク選択肢+1」(PerkChoiceCountLevel>0)を取得していれば4択、
		// 未取得ならデフォルトの3択にする
		data::EnsurePlayerSaveDataLoaded();
		const auto& save = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>().Get();
		select.ChoiceCount = (save.PerkChoiceCountLevel > 0)
			? PerkSelectComponent::kMaxChoiceCount
			: PerkSelectComponent::kDefaultChoiceCount;

		// ── 枠ごとの役割に沿って選択肢を決める ──────────────────────
		// 1番目: 新武器獲得か武器レベルアップかを半々のランダムで決める
		//        (以前は新武器獲得を優先し、出せない場合だけレベルアップにフォールバックしていたが、
		//        常に半々のランダムにしてほしいという要望のため。選んだ方が候補に無ければ
		//        もう一方にフォールバックする)
		// 2番目以降: 新武器獲得・武器レベルアップも含めた全種別をPerkData::Weightによる
		//        重み付き抽選で決める(以前は2番目を武器レベルアップ固定にしていたが、
		//        他のパークと同じ抽選に統合してほしいという要望のため)
		// 既に選んだものは候補から除外し、同じ選択肢が重複して並ばないようにする
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

		// それでも埋まらない場合(候補が選択肢数より少ない)は、
		// 既に提示済みのものを循環させて埋める
		int fallbackSource = -1;
		for (int i = 0; i < select.ChoiceCount; ++i)
		{
			if (select.ChoiceIndices[i] >= 0) { fallbackSource = select.ChoiceIndices[i]; break; }
		}
		if (fallbackSource < 0) return; // 1つも選べなかった(通常発生しない)

		for (int i = 0; i < select.ChoiceCount; ++i)
		{
			if (select.ChoiceIndices[i] < 0) select.ChoiceIndices[i] = fallbackSource;
		}

		// 画面中心(仮想解像度の中央)を基準に、3つの選択肢カードを横へ均等配置する。
		// 中央インデックス(2つ目)のカード中心を画面中心に一致させ、各カードは
		// 「アイコン画像(中心=画面中心の高さ) + その下にテキスト」で構成する。
		const float screenCenterX = static_cast<float>(::sys::Window::Get().GetVirtualWidth()) * 0.5f;
		const float screenCenterY = static_cast<float>(::sys::Window::Get().GetVirtualHeight()) * 0.5f;

		auto& manager = ENTITY_MANAGER;
		auto& textRenderer = ::graphics::TextRenderer::Get();

		// アイコン画像の中心は画面中心の高さに、テキストはその下(アイコン下端 + 余白)に置く
		const float iconCenterY = screenCenterY;
		const float textBaselineY = iconCenterY + kCardSize * 0.5f + kIconTextGapY;

		// 選択肢名の最大行数("\n"区切り、トレードオフパークは2行)を求め、テキスト表示領域の
		// 高さに反映する(1行分しか見込んでいないと2行目がウィンドウ外にはみ出るため)
		int maxOptionTextLines = 1;
		for (int i = 0; i < select.ChoiceCount; ++i)
		{
			const std::wstring& name = pool[select.ChoiceIndices[i]].Name;
			const int lineCount = static_cast<int>(std::count(name.begin(), name.end(), L'\n')) + 1;
			maxOptionTextLines = std::max(maxOptionTextLines, lineCount);
		}
		const float optionTextBlockHeight =
			kOptionTextSize + static_cast<float>(maxOptionTextLines - 1) * textRenderer.MeasureLineHeight(kOptionTextSize);

		// ── 背景(タイトル画面の背景を流用)。黒ウィンドウのさらに奥に全画面で敷く ──
		// パーク選択中は背後の3Dシーンの代わりにこの背景を見せ、メニュー画面らしくする。
		// (スプライトチャンネルは3Dシーンより後に描画されるため、全画面スプライトでシーンを覆える)
		{
			auto bgEntity = manager.CreateEntity();
			auto& bgTransform = manager.AddComponent<ecs::Transform>(bgEntity);
			bgTransform.Set2DPosition(0.0f, 0.0f); // 左上原点(Pivot既定{0,0})で全画面を覆う

			// TitleSceneと同じパス(Windowsは大小無視で実ファイルTX_TItleBG.pngに解決される)
			auto bgTexture = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/Title/TX_TitleBG.png");
			auto& bgSprite = manager.AddComponent<ecs::Sprite>(bgEntity, bgTexture);
			bgSprite.Size = { screenCenterX * 2.0f, screenCenterY * 2.0f };            // 仮想解像度全体
			bgSprite.Color = ::graphics::Color(1.0f, 1.0f, 1.0f, kBackgroundAlpha);    // ほぼ透明(うっすら見える程度)
			bgSprite.SetLayer(::ecs::SpriteLayer::UI, -1);                             // 黒ウィンドウ(offset0)より奥

			// ExitPerkSelectでまとめて破棄させる。TextComponentを持たないためハイライト対象外。
			registry.emplace<PerkOptionUiTag>(bgEntity, -1);
		}

		// 操作説明(1行)のレイアウト。カードのテキスト下端(2行になる場合はその2行目の下端)から
		// 間隔を空けて置き、ウィンドウはこの行を内側に収めるところまで下へ拡張する
		// (以前は操作説明をウィンドウの外(下)に置いてしまい、黒背景から浮いて見えていたための修正)。
		constexpr float kHelpTextSize = 24.0f;
		constexpr float kHelpGapY = 36.0f;       // カードのテキスト下端から操作説明までの間隔(px)
		constexpr float kHelpBottomPadY = 50.0f; // 操作説明の下端からウィンドウ下端までの余白(px)
		const float helpTextY = textBaselineY + optionTextBlockHeight + kHelpGapY;

		// ── 選択肢全体を囲む黒半透明ウィンドウ(アイコンより奥) ──
		// 横: 左右端カードのさらに外側までpad。縦: アイコン上端から操作説明の下端(+pad)までを覆う。
		// windowBottomは操作説明テキストの配置にも使うため、ブロック外の変数に控えておく。
		const float windowTop = iconCenterY - kCardSize * 0.5f - kWindowPadY;
		const float windowBottom = helpTextY + kHelpTextSize + kHelpBottomPadY;
		{
			const float windowWidth =
				kCardSpacingX * static_cast<float>(select.ChoiceCount - 1)
				+ kCardSize + kWindowPadX * 2.0f;

			auto windowEntity = ::ecs::uiutil::CreateTranslucentPanel(
				screenCenterX, (windowTop + windowBottom) * 0.5f,
				windowWidth, windowBottom - windowTop,
				0); // アイコン(offset4)より奥(小さいLayer)

			// ExitPerkSelectでまとめて破棄させる。TextComponentを持たないためハイライト対象外。
			registry.emplace<PerkOptionUiTag>(windowEntity, -1);
		}

		for (int i = 0; i < select.ChoiceCount; ++i)
		{
			const PerkDefinition& perk = pool[select.ChoiceIndices[i]];

			// カード中心X。iを「中央からのオフセット」に変換して横に均等配置(i=1が画面中心)。
			const float indexFromCenter =
				static_cast<float>(i) - static_cast<float>(select.ChoiceCount - 1) * 0.5f;
			const float cardCenterX = screenCenterX + indexFromCenter * kCardSpacingX;

			// 大きめのアイコン画像(中心=画面中心の高さ)。未作成アイコンはGetPerkIconPathが代用画像を返す。
			auto iconEntity = manager.CreateEntity();
			auto& iconTransform = manager.AddComponent<ecs::Transform>(iconEntity);
			iconTransform.Set2DPosition(cardCenterX, iconCenterY);

			auto iconTexture = ::graphics::TextureManager::Get().GetOrLoad(GetPerkIconPath(perk));
			auto& iconSprite = manager.AddComponent<ecs::Sprite>(iconEntity, iconTexture);
			iconSprite.Pivot = { 0.5f, 0.5f };              // Set2DPositionの座標を画像の中心に合わせる
			iconSprite.Size = { kCardSize, kCardSize };     // 元画像サイズに依らず一定の大きさで表示
			iconSprite.SetLayer(::ecs::SpriteLayer::UI, 4); // ウィンドウ(offset0)より手前

			registry.emplace<PerkOptionUiTag>(iconEntity, i);

			// テキスト(アイコンの下)。cardCenterXへ水平中央揃え(MeasureWidthで実幅を測る)。
			// テキストは別パスで描画されるため、アイコン・ウィンドウより常に前面に出る。
			const float textWidth = textRenderer.MeasureWidth(perk.Name, kOptionTextSize);

			auto textEntity = manager.CreateEntity();
			auto& text = manager.AddComponent<TextComponent>(textEntity);
			text.Text = perk.Name;
			text.X = cardCenterX - textWidth * 0.5f;
			text.Y = textBaselineY;
			text.Size = kOptionTextSize;
			text.Color = (i == select.SelectedIndex) ? kSelectedColor : kNormalColor;
			text.Layer = 10;

			// 画像・テキストとも同じタグを付け、ExitPerkSelectでまとめて破棄されるようにする。
			// ハイライト処理(HandleInput)は PerkOptionUiTag+TextComponent のみを見るため、
			// TextComponentを持たないアイコンには影響しない(アイコンは常に通常色で表示)。
			registry.emplace<PerkOptionUiTag>(textEntity, i);
		}

		// ── 操作説明(黒ウィンドウ内、選択肢カードの下中央)。選択肢とは別に常時固定表示する ──
		// 最後に使われた入力デバイスに応じてボタン名を出し分ける(OptionsMenuSystem::
		// BuildControlsInfoLinesと同じくecs::uiutil::CreateTextLinesで生成する)。
		{
			const DirectX::XMFLOAT4 kHelpColor = { 0.85f, 0.85f, 0.85f, 1.0f };

			const ::sys::eInputDevice device = ::sys::InputManager::Get().GetLastInputDevice();
			const std::wstring helpText =
				std::wstring(::ecs::inputguide::GetMenuMoveLabel(device)) + L" : 選択　" +
				std::wstring(::ecs::inputguide::GetSelectLabel(device)) + L" : 決定";

			const auto entities = ::ecs::uiutil::CreateTextLines(
				{ helpText }, ::ecs::uiutil::eTextHorizontalAlign::Center,
				screenCenterX, helpTextY, 0.0f, kHelpTextSize, kHelpColor, 10);

			// 選択肢ではないため-1タグ(ExitPerkSelectでまとめて破棄、ハイライト対象外)
			registry.emplace<PerkOptionUiTag>(entities[0], -1);
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

		// マウスクリックでの誤確定を防ぐため、"Select"のマウス割り当ては無視する
		// (キーボードのSpace/パッドのAボタンのみで決定する)
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

		// パーク確定時の演出(自動選択デバッグ経由も含め、ApplyPerkが呼ばれる箇所すべてで発生する)。
		// レベルアップ〜確定は低頻度のイベントのため、常時ヒットするエフェクトより多少リッチな
		// ものを使う(Herald.efk)。PlayOneShotCombinedの同時再生数上限で暴走はしない
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

#include "apppch.h"
#include "GameSceneFactory.h"
#include "FieldConstants.h"

#include<system/CameraFollow/CameraFollowOffsetComponent.h>
#include<Tag/EntityTag.h>

// プレイヤー
#include<system/Player/State/PlayerStateComponent.h>
#include<system/Player/MovementSystem/PlayerMovementComponent.h>
#include<system/Player/InputSystem/PlayerInputSystem.h>
#include<system/RotateToMove/RotateToMoveComponent.h>
#include<system/MoveDirection/MoveDirectionComponent.h>
#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/UI/FillAmountLerp.h>
#include<Data/StatUpgrade/StatUpgradeData.h>
#include<Data/Save/PlayerSaveData.h>
#include<system/Player/AimSysten/PlayerAimComponent.h>
#include<system/Player/Level/PlayerLevelComponent.h>
#include<system/Player/Ultimate/PlayerUltimateComponent.h>
#include<system/Player/Perk/PlayerPerkLevelComponent.h>
#include<system/Player/Perk/PerkDefinition.h>

// 武器
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponTypeRegistry.h>
#include<system/Player/Weapon/Orbit/OrbitWeaponRuntimeComponent.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeComponent.h>
#include<system/Player/PowerCharge/PlayerPowerChargeComponent.h>
#include<Data/Weapon/FlickerStrikeWeaponData.h>

// 敵
#include<system/Enemy/Move/EnemyChaseComponent.h>
#include<system/Enemy/Attack/EnemyAttackComponent.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Data/Enemy/EnemyData.h>
#include<Data/Enemy/BossData.h>

// UI
#include<system/Player/UI/PlayerUiTag.h>
#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/Window/Window.h> // 必殺ゲージを右端基準へ鏡像配置するため仮想解像度幅を取得する
#include<graphics/Fbx/Resource/FbxResource.h> // FbxResource::FindClipIndex(クリップ重複追加ガード)を使うため

// 状態
#include<Scene/Game/State/GameState.h>
#include<Scene/Game/Wave/WaveComponent.h>
#include<Scene/Game/Wave/WaveTimerUiTag.h>
#include<Data/Wave/WaveData.h>

namespace ecs
{
	void GameSceneFactory::CreateStateController()
	{
		auto& manager = ENTITY_MANAGER;
		auto entity = manager.CreateEntity();

		auto& state = manager.AddComponent<::ecs::GameStateComponent>(entity);
		state.GameState = ::sys::eGameState::InGame;

		// ウェーブサバイバルのコアループ管理（敵の継続スポーン・難易度上昇・ボース出現・クリア判定）
		// 数値はWaveData(CSV/DB、WaveDebugPanelでGUI編集可能)から初期化する
		auto& wave = manager.AddComponent<::ecs::WaveComponent>(entity);
		if (const auto* waveData = DATA_MGR(data::WaveData).GetById(0))
		{
			wave.SpawnInterval = waveData->SpawnInterval;
			wave.SpawnCountPerTick = waveData->SpawnCountPerTick;
			wave.SpawnMarginMin = waveData->SpawnMarginMin;
			wave.SpawnMarginMax = waveData->SpawnMarginMax;
			wave.MaxAliveEnemy = waveData->MaxAliveEnemy;
			wave.StatGrowthStepInterval = waveData->StatGrowthStepInterval;
			wave.StatGrowthPerStep = waveData->StatGrowthPerStep;
			wave.MiniBossFirstSpawnTime = waveData->MiniBossFirstSpawnTime;
			wave.MiniBossInterval = waveData->MiniBossInterval;
			wave.NextMiniBossSpawnTime = waveData->MiniBossFirstSpawnTime; // 初回出現時刻で初期化
			wave.MidBossSpawnTime = waveData->MidBossSpawnTime;
			wave.FinalBossSpawnTime = waveData->FinalBossSpawnTime;
			wave.ClearTime = waveData->ClearTime;
		}

		// TimeScaleはプロセス全体で共有されるシングルトンでシーンをまたいで持ち越される。
		// PerkSelect/ResultはTimeScale=0.0で止めるが、GameStateはPreStartを経由せず
		// 常にここで直接InGameへ入るため(=TimeScale=1.0へ戻すタイミングが他に無いため)、
		// GameOver後のRetry等で0.0のまま残って固まって見えないよう、ここで必ず1.0へ戻す。
		GetTime().SetTimeScale(1.0);
	}

	void GameSceneFactory::CreateBGM()
	{
		// 仮の音楽
		PLAY_BGM("Assets/Sound/BGM/BGM_Title.aud", true, 0.7);
	}

	void GameSceneFactory::CreateGround()
	{
		auto& manager = ENTITY_MANAGER;

		auto res = ::graphics::FbxResourceManager::Get().Load("Assets/Fbx/Field/Field.fbx.bin");

		auto entity = manager.CreateEntity();
		auto& tr = manager.AddComponent<::ecs::Transform>(entity);
		tr.SetScale(FieldConstants::kScale);

		auto& fbx = manager.AddComponent<::ecs::FbxComponent>(entity);
		fbx.Resource = res;
	}

	void GameSceneFactory::CreateFieldBoundary()
	{
		// プレイヤーがフィールド外へ出られないようにする、描画を持たない静的コライダーの壁。
		// FieldConstants::kPlayableHalfExtantの四辺に沿って、内向きの厚みを持つ薄い壁を4枚配置する
		// (見た目はSkyboxで隠れるため、当たり判定だけを持てばよい)。
		auto& manager = ENTITY_MANAGER;

		const float half = FieldConstants::kPlayableHalfExtent;
		const float thickness = FieldConstants::kWallHalfThickness;
		const float height = FieldConstants::kWallHalfHeight;

		// 各壁: {中心座標, 半径(halfExtent)}。壁の長さは隣接する壁の厚みぶん重なるよう
		// (half + thickness)まで伸ばし、四隅に隙間ができないようにする。
		struct WallDef { DirectX::XMFLOAT3 Center; DirectX::XMFLOAT3 HalfExtent; };
		const WallDef walls[4] =
		{
			// +Z側(奥)
			{ { 0.0f, height, half + thickness }, { half + thickness, height, thickness } },
			// -Z側(手前)
			{ { 0.0f, height, -(half + thickness) }, { half + thickness, height, thickness } },
			// +X側(右)
			{ { half + thickness, height, 0.0f }, { thickness, height, half + thickness } },
			// -X側(左)
			{ { -(half + thickness), height, 0.0f }, { thickness, height, half + thickness } },
		};

		for (const auto& wall : walls)
		{
			auto entity = manager.CreateEntity();

			auto& tr = manager.AddComponent<::ecs::Transform>(entity);
			tr.SetPosition(wall.Center);

			manager.AddComponent<::ecs::ColliderComponent>(entity, ::ecs::ColliderComponent::MakeBox(wall.HalfExtent));
			manager.AddComponent<::ecs::RigidBodyComponent>(entity, ::ecs::RigidBodyComponent::MakeStatic());
		}
	}

	void GameSceneFactory::CreateSkybox()
	{
		// フィールド外側(境界壁の向こう)に何もない虚無が見えないよう、キューブマップの空を敷く。
		auto& manager = ENTITY_MANAGER;

		auto entity = manager.CreateEntity();
		auto& skybox = manager.AddComponent<::ecs::SkyboxComponent>(entity);
		skybox.TexturePath = "Assets/Skybox/skybox.dds";
	}

	// プレイヤー
	void GameSceneFactory::CreatePlayer(const CreatePlayerContext& Context)
	{
		// 管理
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;
		auto player_res = ::graphics::FbxResourceManager::Get().Load("Assets/Fbx/Faul/Faul.fbx.bin");

		// Idle/Run アニメーションクリップ(.anm)をFaulリソースへ登録する。
		// ソースは App/Assets/Fbx/Faul/Animation/ の Faul骨格(99ボーン)アニメを FbxConverter で変換したもの
		// (Idle.fbx.anm / Jog.fbx.anm)。Run には Jog を割り当てる。
		//
		// 【FbxConverter 変換フラグ(重要)】ファイル名の後にオプションを付ける:
		//   FbxConverter.exe Idle.fbx --uemodel
		//   FbxConverter.exe Jog.fbx  --uemodel --rootnomove
		// ・--uemodel   : UE(Z-up)由来のFBXの軸をエンジン(Y-up)へ合わせる。付け忘れるとモデルが寝そべる。
		// ・--rootnomove: ルートモーションを除去。移動は物理(RigidBody)制御のため、走りで前へずれないように
		//                 Run(Jog)には必ず付ける。Idleは原地アニメのため不要。
		//
		// 【2026-07-24】以前はコンバータ側の--uemodelにバグ(main.cppでOptionUEModelがデフォルトtrue
		// 固定でフラグの有無が反映されない)があり、正しく変換できなかったため、エンジン側
		// (FbxResource::LoadAnm)に一時的なZ-up→Y-up補正(convertZUpToYUp引数)を追加して回避していた。
		// コンバータのバグを修正した(リポジトリ: ~/source/repos/FbxConverter)ことで、正しいフラグを
		// 付けて変換すれば.anm自体が最初からY-upになるため、根本原因(コンバータ)側で解決している。
		// LoadAnmの第4引数は既定false(補正なし)のままでよい(渡すと二重補正で崩れるので付けないこと)。
		// リソースはbinPathでキャッシュ共有(敵も同じFaulを使う)されるため、シーン再入場での
		// 重複追加を FindClipIndex の既存チェックで防ぐ。
		if (player_res != nullptr)
		{
			constexpr const char* kFaulBin = "Assets/Fbx/Faul/Faul.fbx.bin";
			if (player_res->FindClipIndex("Idle") < 0)
			{
				::graphics::FbxResourceManager::Get().LoadAnm(
					kFaulBin, "Assets/Fbx/Faul/Animation/Idle.fbx.anm", "Idle");
			}
			if (player_res->FindClipIndex("Run") < 0)
			{
				::graphics::FbxResourceManager::Get().LoadAnm(
					kFaulBin, "Assets/Fbx/Faul/Animation/Jog.fbx.anm", "Run");
			}
			// Flicker Strike発動時の単発アクションアニメーション(FlickerStrikeWeaponSystem参照)
			if (player_res->FindClipIndex("Attack_A") < 0)
			{
				::graphics::FbxResourceManager::Get().LoadAnm(
					kFaulBin, "Assets/Fbx/Faul/Animation/Attack_A.fbx.anm", "Attack_A");
			}
		}

		// プレイヤーの生成
		auto p_scale = 0.2f;
		auto player = manager.CreateEntity();

		// 座標系
		auto& tr = manager.AddComponent<ecs::Transform>(player);
		tr.SetScale(p_scale);
		tr.SetPosition(0, 0.1, 0);

		// モデル
		auto& fbx = manager.AddComponent<ecs::FbxComponent>(player);
		fbx.Resource = player_res;

		// アニメーション再生（既定はIdleループ。移動中はLocomotionAnimationSystemがRunへCrossFadeする）
		auto& anim = manager.AddComponent<ecs::FbxAnimComponent>(player);
		if (player_res != nullptr) anim.Play(*player_res, "Idle", true);

		// 入力
		manager.AddComponent<ecs::ColliderComponent>(player, ecs::ColliderComponent::MakeBox({ 5,30,5 }));
		auto& rigid = manager.AddComponent<ecs::RigidBodyComponent>(player, ecs::RigidBodyComponent::MakeDynamic());
		rigid.GravityFactor = 0.0f;
		rigid.LinearDamping = 10.0f;

		// 状態
		auto& state = manager.AddComponent<::ecs::PlayerStateComponent>(player);
		state.AddTransitionMap(::ecs::ePlayerState::Idle, ::ecs::ePlayerState::Move);
		state.AddTransitionMap(::ecs::ePlayerState::Move, ::ecs::ePlayerState::Idle);

		// 移動
		manager.AddComponent<::ecs::PlayerMovementComponent>(player);

		// 回転
		auto& rotate = manager.AddComponent<::ecs::RotateToMoveComponent>(player);
		rotate.InstantRotate = false;
		manager.AddComponent<::ecs::MoveDirectionComponent>(player);

		// ステータス（ゴールドで購入した恒久強化(PlayerSaveData)をBaseへ反映してからRecomputeする）
		auto& status = manager.AddComponent<::ecs::PlayerStatusComponent>(player);
		ApplyStatUpgrades(status);
		status.Recompute();
		status.CurrentHp = status.Current.MaxHp;

		// レベル・経験値（パークシステム用）
		manager.AddComponent<::ecs::PlayerLevelComponent>(player);

		// パーク種別ごとの選択回数(data::PerkData::MaxLevelの上限判定にPerkSelectSystemが使う)
		auto& perkLevel = manager.AddComponent<::ecs::PlayerPerkLevelComponent>(player);
		perkLevel.PickCounts.assign(::ecs::GetPerkPool().size(), 0);

		// 必殺技ゲージ（撃破数で蓄積、満タンで"Ultimate"アクションにより発動可能）
		manager.AddComponent<::ecs::PlayerUltimateComponent>(player);

		// パワーチャージ（撃破数で蓄積、Flicker Strike等のチャージ消費スキルで使用）。
		// 開始時の所持数はFlickerStrikeWeaponData::InitialChargeに従う
		// (パワーチャージの現状唯一の消費先のためここで管理する。MaxChargeと同じ方針)
		auto& powerCharge = manager.AddComponent<::ecs::PlayerPowerChargeComponent>(player);
		if (const auto* flickerStrikeData = DATA_MGR(data::FlickerStrikeWeaponData).GetById(data::kFlickerStrikeGlobalConfigId))
		{
			powerCharge.Count = flickerStrikeData->InitialCharge;
		}
		manager.AddComponent<::ecs::PlayerFlickerStrikeComponent>(player);

		auto& fill = manager.AddComponent<::ecs::FillAmountLerp>(player);
		fill.Speed = 0.5f;

		// 狙い方向（マウス座標/右スティックでPlayerAimSystemが更新する）
		manager.AddComponent<::ecs::PlayerAimComponent>(player);

		registry.emplace<::ecs::PlayerTag>(player);

		manager.AddComponent<::ecs::WeaponInventoryComponent>(player);

		// 初期武器: MenuScene(武器選択)でカーソルを合わせていたSpellMenuData::ID(Context.SelectSpellID)に
		// 対応する武器を1つだけ付与する。IDはAssets/Bin/CSV/MenuSpells.csvの並びと一致させること。
		// WeaponID=0はいずれも対応するCSV(Assets/Data/Weapon/*.csv)のLv1レコードを指す。
		// AddWeaponToPlayer()はパーク選択・デバッグ操作からの武器追加とも共通の経路にしてある。
		// 選ばなかった2種は、パーク選択(PerkSelectSystem::AcquireWeapon)の抽選プールから
		// 入手できる形にする(GetPerkPool参照)。
		switch (Context.SelectSpellID)
		{
		case 1001: // Fire
			AddWeaponToPlayer(player, ::ecs::eWeaponType::SingleShot, 0, ::ecs::eWeaponControl::Manual);  // FireBolt（左クリックで発射）
			break;
		case 1002: // Lightning(内部データはIceSpike/AreaAttackを流用)
			AddWeaponToPlayer(player, ::ecs::eWeaponType::AreaAttack, 0, ::ecs::eWeaponControl::Manual);  // IceSpike（右クリック"Attack2"で発動）
			break;
		case 1003: // Orb
			// SelfDefense(FrostOrb)は発動トリガーの無い常時稼働の武器のためControlは意味を持たないが、区分上はAutoとする
			AddWeaponToPlayer(player, ::ecs::eWeaponType::SelfDefense, 0, ::ecs::eWeaponControl::Auto);
			break;
		default:
			// 未知のIDが渡ってきた場合のフォールバック(GameScene::mSpellIDの既定値1001と同じ武器にしておく)
			AddWeaponToPlayer(player, ::ecs::eWeaponType::SingleShot, 0, ::ecs::eWeaponControl::Manual);
			break;
		}
	}

	entt::entity GameSceneFactory::AddWeaponToPlayer(
		entt::entity player,
		eWeaponType type,
		int weaponId,
		eWeaponControl control)
	{
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;

		auto* inventory = registry.try_get<::ecs::WeaponInventoryComponent>(player);
		if (inventory == nullptr || !inventory->HasFreeSlot())
		{
			return entt::null;
		}

		auto weapon = manager.CreateEntity();
		auto& weaponComp = manager.AddComponent<::ecs::WeaponComponent>(weapon);
		weaponComp.WeaponID = weaponId;
		weaponComp.Type = type;
		weaponComp.MaxLevel = 10; // 全武器共通。各武器のCSVはLv1〜10の10行を持つ(data::XxxWeaponData参照)
		weaponComp.Level = 1;
		weaponComp.Owner = player;
		weaponComp.Control = control;

		// 武器種別ごとのランタイムコンポーネントを付与する(WeaponTypeRegistry参照。
		// 新しい武器種別を追加する場合はそちらのテーブルへ1行追記するだけでよい)
		::ecs::weaponutil::AddWeaponRuntimeComponent(registry, type, weapon);

		inventory->Weapons.push_back(weapon);
		return weapon;
	}

	void GameSceneFactory::RemoveWeaponFromPlayer(entt::entity player, entt::entity weaponEntity)
	{
		auto& registry = ENTT_REGISTRY;

		auto* inventory = registry.try_get<::ecs::WeaponInventoryComponent>(player);
		if (inventory != nullptr)
		{
			auto& weapons = inventory->Weapons;
			weapons.erase(std::remove(weapons.begin(), weapons.end(), weaponEntity), weapons.end());
		}

		// SelfDefense(Orbit)は周回中の子エンティティ(オーブ)を保持しているため、
		// 武器本体だけでなくオーブも合わせて破棄しないと孤立して残り続ける
		if (auto* orbitRuntime = registry.try_get<::ecs::OrbitWeaponRuntimeComponent>(weaponEntity))
		{
			for (entt::entity orb : orbitRuntime->Orbs)
			{
				if (registry.valid(orb)) registry.destroy(orb);
			}
		}

		if (registry.valid(weaponEntity))
		{
			registry.destroy(weaponEntity);
		}
	}

	/// <summary>PlayerSaveData(ゴールドで購入した恒久強化レベル)×StatUpgradeData::ValuePerLevelを
	/// Baseへ加算する。StatUpgradeData::Idはdata::eStatUpgradeTypeと対応する。</summary>
	void GameSceneFactory::ApplyStatUpgrades(ecs::PlayerStatusComponent& status)
	{
		data::EnsurePlayerSaveDataLoaded();
		const auto& save = data::ConfigRegistry::Get().GetManager<data::PlayerSaveData>().Get();

		auto& dataMgr = DATA_MGR(data::StatUpgradeData);

		if (const auto* maxHp = dataMgr.GetById(static_cast<int>(data::eStatUpgradeType::MaxHp)))
		{
			status.Base.MaxHp += maxHp->ValuePerLevel * static_cast<float>(save.MaxHpLevel);
		}
		if (const auto* atkPower = dataMgr.GetById(static_cast<int>(data::eStatUpgradeType::AtkPower)))
		{
			status.Base.AtkPower += atkPower->ValuePerLevel * static_cast<float>(save.AtkPowerLevel);
		}
		if (const auto* defense = dataMgr.GetById(static_cast<int>(data::eStatUpgradeType::Defense)))
		{
			status.Base.Defense += defense->ValuePerLevel * static_cast<float>(save.DefenseLevel);
		}
		if (const auto* cooldownRate = dataMgr.GetById(static_cast<int>(data::eStatUpgradeType::CooldownRate)))
		{
			status.Base.CooldownRate += cooldownRate->ValuePerLevel * static_cast<float>(save.CooldownRateLevel);
		}
		if (const auto* moveSpeed = dataMgr.GetById(static_cast<int>(data::eStatUpgradeType::MoveSpeed)))
		{
			status.Base.MoveSpeed += moveSpeed->ValuePerLevel * static_cast<float>(save.MoveSpeedLevel);
		}
		if (const auto* hpRegen = dataMgr.GetById(static_cast<int>(data::eStatUpgradeType::HpRegen)))
		{
			status.Base.HpRegenPerSecond += hpRegen->ValuePerLevel * static_cast<float>(save.HpRegenLevel);
		}
		// GoldGainRate/ExperienceGainRateはPlayerStatusComponentに接続しない
		// (EnemyDeathSystem::AwardGold/AwardExperienceがPlayerSaveDataを直接参照する)
	}

	void GameSceneFactory::CreateCamera()
	{
		auto& manager = ENTITY_MANAGER;
		auto entity = manager.CreateEntity();

		auto& tr = manager.AddComponent<::ecs::Transform>(entity);

		auto& cam = manager.AddComponent<::ecs::CameraComponent>(entity);
		cam.IsMainCamera = true;
		// POE2のようなアイソメトリック的な見た目にするため、望遠寄りの狭いFOVにして
		// パースの歪み(手前が大きく・奥が小さく見える)を抑える。狭いFOV分、
		// 同程度の画角を保てるようカメラの距離(Offset)を後方・上方へ伸ばしてある。
		cam.Fov = 35.0f;
		cam.Near = 0.1f;
		// 【2026-07-24】フィールド拡張(半径2000、FieldConstants::kWorldHalfExtent)に伴い、
		// 旧値1000のままだとフィールド遠方(や地平線方向のSkybox手前)がFar Clipで
		// 描画されなくなるため、余裕を持って拡張する。
		cam.Far = 3000.0f;
		cam.SetAspectRatioFromWindow(sys::Window::Get());

		auto& follow = manager.AddComponent<::ecs::CameraFollowOffsetComponent>(entity);
		// 俯瞰角度が約70度(水平面基準)で真上に近すぎたとのフィードバックのため、約56度まで戻す
		// (距離感が変わらないようOffsetの大きさはほぼ据え置き、比率のみ変更)
		follow.Offset = { 0.f, 440.f, -300.f };
		follow.LookAtOffset = { 0.f, -10.f, 0.f };

	}

	void GameSceneFactory::CreateDirLight()
	{
		auto& manager = ENTITY_MANAGER;
		entt::entity entity = manager.CreateEntity();

		manager.AddComponent<::ecs::Transform>(entity);

		auto& light = manager.AddComponent<ecs::DirectionalLightComponent>(entity);
		light.Direction = { 0.3f, -1.0f, 0.5f };
		light.Color = { 1.0f,  1.0f, 1.0f };
		light.Intensity = 7.5f;
		light.IsActive = true;
		light.CastShadow = true;

		// 【2026-07-24】フィールド(Field.fbx.bin)は scale=20 適用後で約2000x2000ユニットあるのに対し、
		// ShadowRangeは旧値50(=フィールドの2.5%程度)しかカバーしておらず、プレイヤーが原点から
		// 少し離れるだけでShadow Mapの範囲外に出てしまっていた。SampleShadowPCF(FbxShader.hlsli)は
		// 範囲外を「常に影なし」として扱うため、フィールドの大部分で陰影が付かず、
		// 光がフィールド全体に届いていないように見える不具合になっていた。
		//
		// 対策は2段構え:
		// 1. ShadowTargetをワールド原点固定ではなく、DirLightFollowSystem(PostUpdateフェーズ)で
		//    毎フレームプレイヤー位置(XZ)へ追従させる。これにより影の解像度(2048x2048固定)を
		//    落とさず、プレイヤーが今いる場所では常にシャドウ/陰影が機能する。
		// 2. ShadowRangeも、戦闘中の実射程(VoidBeamのBeamLength=90、AreaAttackのSearchRadius=105等)
		//    を余裕を持ってカバーできるよう150に拡大し、プレイヤー周辺の攻撃エフェクトが
		//    範囲外に出ないようにする。
		light.ShadowRange = 150.0f;
		light.ShadowTarget = { 0.0f, 0.0f, 0.0f }; // 初期値。以降はDirLightFollowSystemが更新する
		light.ShadowDistance = 30.0f;
		light.ShadowNear = 0.1f;
		light.ShadowFar = 200.0f;
		light.ShadowBias = 0.015f;
	}

	void GameSceneFactory::CreateStartEffect()
	{

	}

	void GameSceneFactory::CreateUI()
	{
		// 体力バーのUI
		CreatePlayerHpBar();
		// 必殺技ゲージのUI（体力バーの鏡像。右下）
		CreatePlayerUltimateGauge();
		// 所持武器アイコンバー（体力バーと必殺ゲージの間、画面中央下部）
		CreateWeaponIconBar();
		// 制限時間のUI
		CreateWaveTimerUI();
	}

	void GameSceneFactory::CreateEnemy(
		const DirectX::XMFLOAT3& position,
		const ecs::EnemyWaveModifier& waveModifier,
		eBossTier bossTier,
		int enemyId)
	{
		const bool isBoss = bossTier != eBossTier::None;
		// ボース階級ごとの強化倍率(data::BossData)。通常の敵はnullptrのまま(倍率1.0扱い)
		const auto* bossData = isBoss ? DATA_MGR(data::BossData).GetById(static_cast<int>(bossTier)) : nullptr;

		// 敵の種類ごとの色分け（専用モデルが用意されるまではプレイヤーモデルの色違いで代用する）。
		// ボースは階級ごとにこの色を優先する(種類に関わらず、同じ階級なら同じ色)。
		constexpr DirectX::XMFLOAT4 kMiniBossColor = { 1.0f, 0.5f, 0.1f, 1.0f };  // 小ボース: 橙
		constexpr DirectX::XMFLOAT4 kMidBossColor = { 1.0f, 0.15f, 0.15f, 1.0f }; // 中ボース: 赤
		constexpr DirectX::XMFLOAT4 kFinalBossColor = { 0.55f, 0.05f, 0.65f, 1.0f }; // 最強ボース: 紫
		constexpr DirectX::XMFLOAT4 kGruntColor = { 1.0f, 1.0f, 0.5f, 1.0f };     // Id=0: 標準的な敵
		constexpr DirectX::XMFLOAT4 kScoutColor = { 0.4f, 0.9f, 1.0f, 1.0f };    // Id=1: 高速・低HPな敵
		constexpr DirectX::XMFLOAT4 kBruteColor = { 0.6f, 0.1f, 0.5f, 1.0f };    // Id=2: 低速・高HP・高火力な敵
		constexpr DirectX::XMFLOAT4 kSprinterColor = { 0.5f, 1.0f, 0.3f, 1.0f }; // Id=3: 超高速・超低HPな敵

		const auto* enemyData = DATA_MGR(data::EnemyData).GetById(enemyId);

		// 管理
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;
		auto res = ::graphics::FbxResourceManager::Get().Load("Assets/Fbx/Faul/Faul.fbx.bin");

		// 敵の生成
		const float baseScale = 0.2f;
		const float bossScaleMultiplier = bossData != nullptr ? bossData->ScaleMultiplier : 1.0f;
		const float scale = baseScale * bossScaleMultiplier;
		auto enemy = manager.CreateEntity();

		// 座標系
		auto& tr = manager.AddComponent<ecs::Transform>(enemy);
		tr.SetScale(scale);
		tr.SetPosition(position);

		// 物理（コライダーは見た目のスケールに追従しないため、ボースは箱も合わせて拡大する）
		const DirectX::XMFLOAT3 colliderHalfExtent = isBoss
			? DirectX::XMFLOAT3{ 5.0f * bossScaleMultiplier, 30.0f * bossScaleMultiplier, 5.0f * bossScaleMultiplier }
			: DirectX::XMFLOAT3{ 5.0f, 30.0f, 5.0f };
		manager.AddComponent<ecs::ColliderComponent>(enemy, ecs::ColliderComponent::MakeBox(colliderHalfExtent));
		auto& rigid = manager.AddComponent<ecs::RigidBodyComponent>(enemy, ecs::RigidBodyComponent::MakeDynamic());
		rigid.GravityFactor = 0.0f;
		rigid.LinearDamping = 10.0f;
		// 敵同士は衝突させない(見た目上の押し合いはほぼ不要な一方、敵が密集すると
		// Joltの接触解決コストが急増しFPS低下の主因になるため)。地面・プレイヤー・
		// 各武器のセンサー判定とは通常通り衝突する
		rigid.DisableSelfCollision = true;

		// モデル（専用モデルが用意されるまではプレイヤーモデルを色違いで代用する）
		auto& fbx = manager.AddComponent<ecs::FbxComponent>(enemy);
		fbx.Resource = res;

		DirectX::XMFLOAT4 enemyColor = kGruntColor;
		switch (bossTier)
		{
		case eBossTier::Mini:  enemyColor = kMiniBossColor; break;
		case eBossTier::Mid:   enemyColor = kMidBossColor; break;
		case eBossTier::Final: enemyColor = kFinalBossColor; break;
		case eBossTier::None:
		default:
			switch (enemyId)
			{
			case 1: enemyColor = kScoutColor; break;
			case 2: enemyColor = kBruteColor; break;
			case 3: enemyColor = kSprinterColor; break;
			default: break; // Id=0またはその他は標準色(kGruntColor)のまま
			}
			break;
		}
		fbx.CustomColor = enemyColor;

		// アニメーション再生（既定はIdleループ。移動中はLocomotionAnimationSystemがRunへCrossFadeする）。
		// Idle/Runクリップは既にCreatePlayerがFaulリソースへ登録済み(敵は常にプレイヤー生成後に
		// EnemySpawnSystemが生成するため、ここで再度LoadAnmを呼ぶ必要はない)。
		auto& anim = manager.AddComponent<ecs::FbxAnimComponent>(enemy);
		if (res != nullptr) anim.Play(*res, "Idle", true);

		// 移動（EnemyData.csvのMoveSpeedを使用。プレイヤーの実移動速度
		// PlayerMovementComponent::MaxSpeed(110)より遅くなるよう、各敵種のMoveSpeedを調整すること）
		auto& chase = manager.AddComponent<::ecs::EnemyChaseComponent>(enemy);
		chase.MoveSpeed = enemyData != nullptr ? enemyData->MoveSpeed : 60.0f;
		auto& rotate = manager.AddComponent<::ecs::RotateToMoveComponent>(enemy);
		rotate.InstantRotate = false;
		manager.AddComponent<::ecs::MoveDirectionComponent>(enemy);

		// ステータス（EnemyData.csvの種類別ステータスを基準値とし、現在の難易度倍率を適用。
		// ボースはさらに追加倍率をかける）
		auto& status = manager.AddComponent<::ecs::EnemyStatusComponent>(enemy);
		status.EnemyId = enemyId;
		if (enemyData != nullptr)
		{
			status.Base.MaxHp = enemyData->MaxHp;
			status.Base.AtkPower = enemyData->AtkPower;
			status.Base.ExperienceValue = static_cast<float>(enemyData->Exp);
			status.Base.GoldValue = enemyData->GoldValue;
		}
		status.WaveMod = waveModifier;
		if (bossData != nullptr)
		{
			status.WaveMod.MulMaxHp *= bossData->HpMultiplier;
			status.WaveMod.MulAtkPower *= bossData->AtkMultiplier;
			status.Base.ExperienceValue *= bossData->ExpMultiplier;
			status.Base.GoldValue *= bossData->GoldMultiplier;
		}
		status.Recompute();
		status.CurrentHp = status.Current.MaxHp;

		// 攻撃
		auto& attack = manager.AddComponent<::ecs::EnemyAttackComponent>(enemy);

		registry.emplace<::ecs::EnemyTag>(enemy);
	}

	void GameSceneFactory::CreatePlayerHpBar()
	{
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;
		float scale = 0.6;
		// 座標
		float offcet = 190;
		float pos_y = 900;

		// ベースの作成
		{
			auto entity = manager.CreateEntity();
			auto res = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/UI/HpBar/bar_base.png");

			auto& tr = manager.AddComponent<ecs::Transform>(entity);
			tr.Set2DScale({scale,scale});
			tr.Set2DPosition(0, pos_y);

			auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, res);
			sprite.SetLayer(::ecs::SpriteLayer::UI,2);
			sprite.Intensity = 1.5f;

		}

		// 本体の作成
		{
			auto entity = manager.CreateEntity();
			auto res = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/UI/HpBar/bar1.png");

			auto& tr = manager.AddComponent<ecs::Transform>(entity);
			tr.Set2DScale({ scale,scale });
			tr.Set2DPosition(offcet, pos_y);


			auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, res);
			sprite.SetLayer(::ecs::SpriteLayer::UI, 1);
			sprite.Intensity = 5.0f;

			auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
			glow.Amplitude = 2;
			glow.BaseIntensity = 5;
			glow.Frequency = 0.5;
			glow.PhaseOffset = 0.0f;

			registry.emplace<::ecs::PlayerHpBarTag>(entity);

		}

	}

	void GameSceneFactory::CreatePlayerUltimateGauge()
	{
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;

		// 体力バーと同じ画像・スケール・高さを流用し、左右反転して右下へ鏡像配置する。
		// 反転は Flip.x=-1 で行い、X座標を「仮想解像度幅 - 元のX」へ置き換えることで、
		// 左端基準だったスプライトを右端基準へ移す(SpriteRenderer::CalculateShaderData の
		// mPivot→mScale(Flip)→translate の順により、Flip.x=-1 では基準点が右端になる)。
		// 左下=HP / 右下=必殺 で左右対称の配置になる。
		const float virtualWidth = static_cast<float>(::sys::Window::Get().GetVirtualWidth());

		// 体力バー(CreatePlayerHpBar)と同じレイアウト値。鏡像なので offset は右端からの距離になる
		const float scale = 0.6f;
		const float offset = 190.0f;
		const float pos_y = 900.0f;

		// 必殺ゲージの識別色(青)。体力バー(赤系)と一目で区別できるようにする
		const ::graphics::Color kGaugeColor = ::graphics::Color::Blue;

		// ベースの作成(元X=0 の鏡像 → 右端 virtualWidth に合わせる)
		{
			auto entity = manager.CreateEntity();
			auto res = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/UI/HpBar/bar_base.png");

			auto& tr = manager.AddComponent<ecs::Transform>(entity);
			tr.Set2DScale({ scale, scale });
			tr.Set2DPosition(virtualWidth, pos_y);

			auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, res);
			sprite.SetLayer(::ecs::SpriteLayer::UI, 2);
			sprite.Intensity = 1.5f;
			sprite.Flip.x = -1.0f;      // 左右反転
			sprite.Color = kGaugeColor; // 青
		}

		// 本体(ゲージ)の作成(元X=offset の鏡像 → 右端から offset 内側へ)
		{
			auto entity = manager.CreateEntity();
			auto res = ::graphics::TextureManager::Get().GetOrLoad("Assets/Texture/UI/HpBar/bar1.png");

			auto& tr = manager.AddComponent<ecs::Transform>(entity);
			tr.Set2DScale({ scale, scale });
			tr.Set2DPosition(virtualWidth - offset, pos_y);

			auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, res);
			sprite.SetLayer(::ecs::SpriteLayer::UI, 1);
			sprite.Intensity = 5.0f;
			sprite.Flip.x = -1.0f;      // 左右反転
			sprite.Color = kGaugeColor; // 青
			// 開始時ゲージは空。以降 PlayerUltimateGaugeSystem が撃破数の充填率で更新する
			sprite.FillAmount = 0.0f;

			auto& glow = manager.AddComponent<::ecs::GlowAnimation>(entity);
			glow.Amplitude = 2.0f;
			glow.BaseIntensity = 5.0f;
			glow.Frequency = 0.5f;
			glow.PhaseOffset = 0.0f;

			// FillAmount を目標値へ滑らかに追従させる(撃破のたびにゲージが滑らかに増える)
			auto& lerp = manager.AddComponent<::ecs::FillAmountLerp>(entity);
			lerp.Target = 0.0f;
			lerp.Speed = 2.0f;

			registry.emplace<::ecs::PlayerUltimateGaugeTag>(entity);
		}
	}

	void GameSceneFactory::CreateWeaponIconBar()
	{
		// 体力バー・必殺ゲージ(いずれもpos_y=900)の間、画面中央下寄りに横5×縦2で所持武器アイコンを
		// 並べる。各スロットは「アイコン本体」「クールダウン進捗を表す黒半透明オーバーレイ
		// (Radial FillTypeで時計回りに消える)」「残り秒数のテキスト」「武器レベルのテキスト」の
		// 4エンティティで構成し、実際の表示切り替え(所持武器の反映・クールダウン計算)は
		// WeaponIconBarSystemが行う。ここでは10スロット分を先に生成し、空きスロットは
		// 非表示(IsVisible=false)にしておく(毎フレームのエンティティ生成/破棄を避けるため)。
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;

		constexpr int kColumns = 5;
		constexpr int kRows = 2;
		// WeaponIconBarSystem::kSlotCountと一致させること(WeaponInventoryComponent::MaxSlotsと同数)
		constexpr int kSlotCount = kColumns * kRows;

		constexpr float kIconSize = 76.0f;  // アイコンサイズ
		constexpr float kSpacing = 94.0f;   // スロット中心どうしの間隔(px)。アイコンサイズに応じて隙間を保つ
		constexpr float kCenterY = 945.0f;  // 体力バー・必殺ゲージ(pos_y=900)より少し下

		// 武器レベルのテキストをアイコン右下に重ねるためのオフセット
		constexpr float kLevelTextOffset = kIconSize * 0.32f;

		const float centerX = static_cast<float>(::sys::Window::Get().GetVirtualWidth()) * 0.5f;

		// クールダウン進捗の黒半透明オーバーレイ用の単色板(パーク選択のウィンドウ背景と同じ白テクスチャ)
		constexpr const char* kOverlayTexturePath = "Assets/Effect/Texture/White.png";
		// 空きスロット状態(IsVisible=false)の間だけ使うダミーテクスチャ
		constexpr const char* kPlaceholderIconPath = "Assets/Icon/loading.png";

		for (int slot = 0; slot < kSlotCount; ++slot)
		{
			const int col = slot % kColumns;
			const int row = slot / kColumns;
			const float x = centerX + (static_cast<float>(col) - (kColumns - 1) * 0.5f) * kSpacing;
			const float y = kCenterY + (static_cast<float>(row) - (kRows - 1) * 0.5f) * kSpacing;

			// アイコン本体(武器種別に応じたテクスチャはWeaponIconBarSystemが差し替える)
			{
				auto entity = manager.CreateEntity();
				auto& tr = manager.AddComponent<ecs::Transform>(entity);
				tr.Set2DPosition(x, y);

				auto tex = ::graphics::TextureManager::Get().GetOrLoad(kPlaceholderIconPath);
				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, tex);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { kIconSize, kIconSize };
				sprite.SetLayer(::ecs::SpriteLayer::UI, 3);
				sprite.IsVisible = false; // 初期状態は空きスロット扱い

				registry.emplace<::ecs::WeaponIconSlotTag>(entity,
					::ecs::WeaponIconSlotTag{ slot, ::ecs::eWeaponIconElement::Icon, x });
			}

			// クールダウン進捗オーバーレイ(アイコンの真上に重ねる)
			{
				auto entity = manager.CreateEntity();
				auto& tr = manager.AddComponent<ecs::Transform>(entity);
				tr.Set2DPosition(x, y);

				auto tex = ::graphics::TextureManager::Get().GetOrLoad(kOverlayTexturePath);
				auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, tex);
				sprite.Pivot = { 0.5f, 0.5f };
				sprite.Size = { kIconSize, kIconSize };
				sprite.Color = ::graphics::Color(0.0f, 0.0f, 0.0f, 0.65f); // 黒・65%不透明
				sprite.FType = ::ecs::FillType::Radial; // 時計回りにクールダウン経過を表現する
				sprite.FillAmount = 0.0f;
				sprite.SetLayer(::ecs::SpriteLayer::UI, 4); // アイコン(offset3)より手前
				sprite.IsVisible = false;

				registry.emplace<::ecs::WeaponIconSlotTag>(entity,
					::ecs::WeaponIconSlotTag{ slot, ::ecs::eWeaponIconElement::Overlay, x });
			}

			// 残りクールダウン秒数のテキスト(アイコン中央に重ねる。別描画パスのため常に最前面)
			{
				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<ecs::TextComponent>(entity);
				text.Text = L"";
				text.X = x; // WeaponIconBarSystemがMeasureWidthで中央揃えに書き換える
				text.Y = y;
				text.Size = 20.0f; // 小さいアイコンに合わせた控えめなサイズ
				text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
				text.Layer = 10;
				text.IsVisible = false;

				registry.emplace<::ecs::WeaponIconSlotTag>(entity,
					::ecs::WeaponIconSlotTag{ slot, ::ecs::eWeaponIconElement::Text, x });
			}

			// 武器レベルのテキスト(アイコン右下に重ねる。常時表示、クールダウンとは独立)
			{
				const float levelX = x + kLevelTextOffset;
				const float levelY = y + kLevelTextOffset;

				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<ecs::TextComponent>(entity);
				text.Text = L"";
				text.X = levelX; // WeaponIconBarSystemがMeasureWidthで中央揃えに書き換える
				text.Y = levelY;
				text.Size = 22.0f; // クールダウン秒数(20.0f)より少し大きく、視認性を優先する
				text.Color = { 1.0f, 0.9f, 0.4f, 1.0f }; // 淡い黄色でクールダウン秒数(白)と区別する
				text.Layer = 10;
				text.IsVisible = false;

				registry.emplace<::ecs::WeaponIconSlotTag>(entity,
					::ecs::WeaponIconSlotTag{ slot, ::ecs::eWeaponIconElement::LevelText, levelX });
			}
		}
	}

	void GameSceneFactory::CreateWaveTimerUI()
	{
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;

		auto entity = manager.CreateEntity();

		auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
		text.Text = L"00:00"; // WaveTimerUiSystem が毎フレーム上書きする
		text.X = 880.0f;
		text.Y = 30.0f;
		text.Size = 48.0f;
		text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		text.Layer = 1;

		registry.emplace<::ecs::WaveTimerUiTag>(entity);
	}

}
  
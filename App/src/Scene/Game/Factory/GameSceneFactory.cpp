#include "apppch.h"
#include "GameSceneFactory.h"
#include "FieldConstants.h"

#include<system/CameraFollow/CameraFollowOffsetComponent.h>
#include<Tag/EntityTag.h>

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

#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponTypeRegistry.h>
#include<system/Player/Weapon/Orbit/OrbitWeaponRuntimeComponent.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeComponent.h>
#include<system/Player/PowerCharge/PlayerPowerChargeComponent.h>
#include<Data/Weapon/FlickerStrikeWeaponData.h>

#include<system/Enemy/Move/EnemyChaseComponent.h>
#include<system/Enemy/Attack/EnemyAttackComponent.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Data/Enemy/EnemyData.h>
#include<Data/Enemy/BossData.h>

#include<system/Player/UI/PlayerUiTag.h>
#include<system/GlowAnimation/GlowAnimationComp.h>
#include<system/Window/Window.h>
#include<graphics/Fbx/Resource/FbxResource.h>

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

		// コアループ管理。数値はWaveDataから初期化する
		auto& wave = manager.AddComponent<::ecs::WaveComponent>(entity);
		if (const auto* waveData = DATA_MGR(data::WaveData).GetById(0))
		{
			wave.SpawnInterval = waveData->SpawnInterval;
			wave.SpawnCountPerTick = waveData->SpawnCountPerTick;
			wave.SpawnMarginMin = waveData->SpawnMarginMin;
			wave.SpawnMarginMax = waveData->SpawnMarginMax;
			wave.SpawnCountGrowthStepInterval = waveData->SpawnCountGrowthStepInterval;
			wave.SpawnCountGrowthPerStep = waveData->SpawnCountGrowthPerStep;
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

		// TimeScaleはプロセス全体で共有されるため、GameOver後のRetry等で0のまま固まらないようここで1.0へ戻す
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
		// プレイヤーがフィールド外へ出られないよう、描画を持たない静的コライダーの壁を4枚配置する
		auto& manager = ENTITY_MANAGER;

		const float half = FieldConstants::kPlayableHalfExtent;
		const float thickness = FieldConstants::kWallHalfThickness;
		const float height = FieldConstants::kWallHalfHeight;

		// 各壁は中心座標と半径のペア。隣接壁の厚み分重ねて四隅の隙間を防ぐ
		struct WallDef { DirectX::XMFLOAT3 Center; DirectX::XMFLOAT3 HalfExtent; };
		const WallDef walls[4] =
		{
			// +Z側、奥
			{ { 0.0f, height, half + thickness }, { half + thickness, height, thickness } },
			// -Z側、手前
			{ { 0.0f, height, -(half + thickness) }, { half + thickness, height, thickness } },
			// +X側、右
			{ { half + thickness, height, 0.0f }, { thickness, height, half + thickness } },
			// -X側、左
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
		// フィールド外側に何もない虚無が見えないよう、キューブマップの空を敷く
		auto& manager = ENTITY_MANAGER;

		auto entity = manager.CreateEntity();
		auto& skybox = manager.AddComponent<::ecs::SkyboxComponent>(entity);
		skybox.TexturePath = "Assets/Skybox/skybox.dds";
	}

	void GameSceneFactory::CreatePlayer(const CreatePlayerContext& Context)
	{
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;
		auto player_res = ::graphics::FbxResourceManager::Get().Load("Assets/Fbx/Faul/Faul.fbx.bin");

		// Idle/Runアニメーションをリソースへ登録する。FbxConverterは--uemodelと--rootnomove付きで変換すること
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
			// Flicker Strike発動時の単発アクションアニメーション、FlickerStrikeWeaponSystem参照
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

		// アニメーション再生。既定はIdleループ、移動中はLocomotionAnimationSystemがRunへCrossFadeする
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

		// ステータス。ゴールドで購入した恒久強化をBaseへ反映してからRecomputeする
		auto& status = manager.AddComponent<::ecs::PlayerStatusComponent>(player);
		ApplyStatUpgrades(status);
		status.Recompute();
		status.CurrentHp = status.Current.MaxHp;

		// レベル・経験値、パークシステム用
		manager.AddComponent<::ecs::PlayerLevelComponent>(player);

		// パーク種別ごとの選択回数。data::PerkData::MaxLevelの上限判定にPerkSelectSystemが使う
		auto& perkLevel = manager.AddComponent<::ecs::PlayerPerkLevelComponent>(player);
		perkLevel.PickCounts.assign(::ecs::GetPerkPool().size(), 0);

		// 必殺技ゲージ。撃破数で蓄積し満タンでUltimateアクションにより発動可能
		manager.AddComponent<::ecs::PlayerUltimateComponent>(player);

		// パワーチャージ。撃破数で蓄積しFlicker Strike等のチャージ消費スキルで使用、開始所持数はFlickerStrikeWeaponData::InitialChargeに従う
		auto& powerCharge = manager.AddComponent<::ecs::PlayerPowerChargeComponent>(player);
		if (const auto* flickerStrikeData = DATA_MGR(data::FlickerStrikeWeaponData).GetById(data::kFlickerStrikeGlobalConfigId))
		{
			powerCharge.Count = flickerStrikeData->InitialCharge;
		}
		manager.AddComponent<::ecs::PlayerFlickerStrikeComponent>(player);

		auto& fill = manager.AddComponent<::ecs::FillAmountLerp>(player);
		fill.Speed = 0.5f;

		// 狙い方向。マウス座標または右スティックでPlayerAimSystemが更新する
		manager.AddComponent<::ecs::PlayerAimComponent>(player);

		registry.emplace<::ecs::PlayerTag>(player);

		manager.AddComponent<::ecs::WeaponInventoryComponent>(player);

		// 初期武器はMenuSceneで選んだ武器をLv3、他2種をLv1で3種とも付与する。AddWeaponToPlayerはパーク選択・デバッグ操作とも共通の経路
		constexpr int kSelectedWeaponStartLevel = 3;

		const entt::entity fireWeapon = AddWeaponToPlayer(player, ::ecs::eWeaponType::SingleShot, 0, ::ecs::eWeaponControl::Manual);   // Fire: FireBolt、左クリックで発射
		const entt::entity lightningWeapon = AddWeaponToPlayer(player, ::ecs::eWeaponType::AreaAttack, 0, ::ecs::eWeaponControl::Manual); // Lightning: IceSpike、右クリックAttack2で発動
		// SelfDefenseのFrostOrbは発動トリガーが無くControlは意味を持たないが、区分上はAutoとする
		const entt::entity orbWeapon = AddWeaponToPlayer(player, ::ecs::eWeaponType::SelfDefense, 0, ::ecs::eWeaponControl::Auto);      // Orb: FrostOrb

		entt::entity selectedWeapon = fireWeapon; // 未知のIDが渡ってきた場合のフォールバック
		switch (Context.SelectSpellID)
		{
		case 1001: selectedWeapon = fireWeapon;      break;
		case 1002: selectedWeapon = lightningWeapon; break;
		case 1003: selectedWeapon = orbWeapon;       break;
		default: break;
		}

		if (registry.valid(selectedWeapon))
		{
			registry.get<::ecs::WeaponComponent>(selectedWeapon).Level = kSelectedWeaponStartLevel;
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
		weaponComp.MaxLevel = 10; // 全武器共通。各武器のCSVはLv1~10の10行を持つ、data::XxxWeaponData参照
		weaponComp.Level = 1;
		weaponComp.Owner = player;
		weaponComp.Control = control;

		// 武器種別ごとのランタイムコンポーネントを付与する。新しい武器種別はWeaponTypeRegistryへ1行追記するだけでよい
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

		// SelfDefenseのOrbitは周回中の子オーブを保持しているため、武器と一緒に破棄しないと孤立して残る
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
		if (const auto* attackCount = dataMgr.GetById(static_cast<int>(data::eStatUpgradeType::AttackCount)))
		{
			// パークのAttackCountUpと同じMulAttackCountへ直接加算する。Recomputeは呼び出し元が別途行う
			status.Modifier.MulAttackCount += attackCount->ValuePerLevel * static_cast<float>(save.AttackCountLevel);
		}
		if (const auto* revive = dataMgr.GetById(static_cast<int>(data::eStatUpgradeType::Revive)))
		{
			status.ReviveCount += static_cast<int>(std::lround(revive->ValuePerLevel * static_cast<float>(save.ReviveLevel)));
		}
		if (const auto* postHitInvincibility = dataMgr.GetById(static_cast<int>(data::eStatUpgradeType::PostHitInvincibility)))
		{
			status.Base.PostHitInvincibleDuration += postHitInvincibility->ValuePerLevel * static_cast<float>(save.PostHitInvincibilityLevel);
		}
		// GoldGainRate/ExperienceGainRate/PerkChoiceCountはPlayerStatusComponentに接続せず、EnemyDeathSystem/PerkSelectSystemがPlayerSaveDataを直接参照する
	}

	void GameSceneFactory::CreateCamera()
	{
		auto& manager = ENTITY_MANAGER;
		auto entity = manager.CreateEntity();

		auto& tr = manager.AddComponent<::ecs::Transform>(entity);

		auto& cam = manager.AddComponent<::ecs::CameraComponent>(entity);
		cam.IsMainCamera = true;
		// POE2のようなアイソメトリックの見た目にするため望遠寄りの狭いFOVにし、同じ画角を保てるようOffsetを後方・上方へ伸ばしてある
		cam.Fov = 35.0f;
		cam.Near = 0.1f;
		// フィールド拡張に伴い、Far Clipで地平線方向のSkyboxが描画されなくなるため余裕を持って拡張する
		cam.Far = 3000.0f;
		cam.SetAspectRatioFromWindow(sys::Window::Get());

		auto& follow = manager.AddComponent<::ecs::CameraFollowOffsetComponent>(entity);
		// 俯瞰角度が真上に近すぎるとのフィードバックのため56度まで戻す。距離感を保つためOffsetの比率のみ変更
		follow.Offset = { 0.f, 440.f, -300.f };
		follow.LookAtOffset = { 0.f, -10.f, 0.f };

	}

	void GameSceneFactory::CreateDirLight()
	{
		auto& manager = ENTITY_MANAGER;
		entt::entity entity = manager.CreateEntity();

		manager.AddComponent<::ecs::Transform>(entity);

		auto& light = manager.AddComponent<ecs::DirectionalLightComponent>(entity);
		// 影はDirectionの水平成分の向きへ伸びる。カメラが手前を向くよう水平成分の符号を反転している
		light.Direction = { -0.3f, -1.0f, -0.5f };
		light.Color = { 1.0f,  1.0f, 1.0f };
		light.Intensity = 7.5f;
		light.IsActive = true;
		light.CastShadow = true;

		// ShadowTargetはDirLightFollowSystemが毎フレームプレイヤー位置へ追従させ、ShadowRangeは戦闘の実射程を余裕を持ってカバーする
		light.ShadowRange = 300.0f;
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
		// 必殺技ゲージのUI、体力バーの鏡像で右下
		CreatePlayerUltimateGauge();
		// 所持武器アイコンバー、体力バーと必殺ゲージの間の画面中央下部
		CreateWeaponIconBar();
		// 制限時間のUI
		CreateWaveTimerUI();
		// 経験値バー・現在レベル表示のUI、画面上部
		CreatePlayerExpBar();
	}

	void GameSceneFactory::CreateEnemy(
		const DirectX::XMFLOAT3& position,
		const ecs::EnemyWaveModifier& waveModifier,
		eBossTier bossTier,
		int enemyId)
	{
		const bool isBoss = bossTier != eBossTier::None;
		// ボス階級ごとの強化倍率、data::BossData。通常の敵はnullptrのまま倍率1.0扱い
		const auto* bossData = isBoss ? DATA_MGR(data::BossData).GetById(static_cast<int>(bossTier)) : nullptr;

		// 敵の種類ごとに色分けする。専用モデルが無いためプレイヤーモデルの色違いで代用し、ボスは階級ごとの色を優先する
		constexpr DirectX::XMFLOAT4 kMiniBossColor = { 1.0f, 0.5f, 0.1f, 1.0f };  // 小ボス、橙
		constexpr DirectX::XMFLOAT4 kMidBossColor = { 1.0f, 0.15f, 0.15f, 1.0f }; // 中ボス、赤
		constexpr DirectX::XMFLOAT4 kFinalBossColor = { 0.55f, 0.05f, 0.65f, 1.0f }; // 最強ボス、紫
		constexpr DirectX::XMFLOAT4 kGruntColor = { 1.0f, 1.0f, 0.5f, 1.0f };     // Id=0、標準的な敵
		constexpr DirectX::XMFLOAT4 kScoutColor = { 0.4f, 0.9f, 1.0f, 1.0f };    // Id=1、高速・低HPな敵
		constexpr DirectX::XMFLOAT4 kBruteColor = { 0.6f, 0.1f, 0.5f, 1.0f };    // Id=2、低速・高HP・高火力な敵
		constexpr DirectX::XMFLOAT4 kSprinterColor = { 0.5f, 1.0f, 0.3f, 1.0f }; // Id=3、超高速・超低HPな敵

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

		// 物理。コライダーは見た目のスケールに追従しないためボスは箱も合わせて拡大する
		const DirectX::XMFLOAT3 colliderHalfExtent = isBoss
			? DirectX::XMFLOAT3{ 5.0f * bossScaleMultiplier, 30.0f * bossScaleMultiplier, 5.0f * bossScaleMultiplier }
			: DirectX::XMFLOAT3{ 5.0f, 30.0f, 5.0f };
		manager.AddComponent<ecs::ColliderComponent>(enemy, ecs::ColliderComponent::MakeBox(colliderHalfExtent));
		auto& rigid = manager.AddComponent<ecs::RigidBodyComponent>(enemy, ecs::RigidBodyComponent::MakeDynamic());
		rigid.GravityFactor = 0.0f;
		rigid.LinearDamping = 10.0f;
		// 敵同士は衝突させない。密集時のJolt接触解決コストによるFPS低下を防ぐため
		rigid.DisableSelfCollision = true;

		// モデル。専用モデルが無いためプレイヤーモデルを色違いで代用する
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
			default: break; // Id=0またはその他は標準色kGruntColorのまま
			}
			break;
		}
		fbx.CustomColor = enemyColor;

		// アニメーション再生。既定はIdleループで移動中はRunへCrossFadeし、クリップはCreatePlayerで登録済みのため再登録不要
		auto& anim = manager.AddComponent<ecs::FbxAnimComponent>(enemy);
		if (res != nullptr) anim.Play(*res, "Idle", true);

		// 移動。EnemyData.csvのMoveSpeedを使用し、プレイヤーの実移動速度より遅くなるよう調整すること
		auto& chase = manager.AddComponent<::ecs::EnemyChaseComponent>(enemy);
		chase.MoveSpeed = enemyData != nullptr ? enemyData->MoveSpeed : 60.0f;
		auto& rotate = manager.AddComponent<::ecs::RotateToMoveComponent>(enemy);
		rotate.InstantRotate = false;
		manager.AddComponent<::ecs::MoveDirectionComponent>(enemy);

		// ステータス。EnemyData.csvの種類別値に難易度倍率を適用し、ボスはさらに追加倍率をかける
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

		// 体力バーと同じ画像をFlip.xで左右反転し、X座標を仮想解像度幅基準に置き換えて右下へ鏡像配置する
		const float virtualWidth = static_cast<float>(::sys::Window::Get().GetVirtualWidth());

		// 体力バーと同じレイアウト値。鏡像のためoffsetは右端からの距離になる
		const float scale = 0.6f;
		const float offset = 190.0f;
		const float pos_y = 900.0f;

		// 必殺ゲージの識別色は青。体力バーの赤系と一目で区別できるようにする
		const ::graphics::Color kGaugeColor = ::graphics::Color::Blue;

		// ベースの作成、元X=0の鏡像を右端virtualWidthに合わせる
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

		// 本体ゲージの作成、元X=offsetの鏡像を右端からoffset内側へ
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

			// FillAmountを目標値へ滑らかに追従させる。撃破のたびにゲージが滑らかに増える
			auto& lerp = manager.AddComponent<::ecs::FillAmountLerp>(entity);
			lerp.Target = 0.0f;
			lerp.Speed = 2.0f;

			registry.emplace<::ecs::PlayerUltimateGaugeTag>(entity);
		}
	}

	void GameSceneFactory::CreateWeaponIconBar()
	{
		// 体力バーと必殺ゲージの間に横5×縦2で所持武器アイコンを並べ、表示切り替えはWeaponIconBarSystemが行う
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;

		constexpr int kColumns = 5;
		constexpr int kRows = 2;
		// WeaponIconBarSystem::kSlotCountと一致させること。WeaponInventoryComponent::MaxSlotsと同数
		constexpr int kSlotCount = kColumns * kRows;

		constexpr float kIconSize = 76.0f;  // アイコンサイズ
		constexpr float kSpacing = 94.0f;   // スロット中心どうしの間隔、px。アイコンサイズに応じて隙間を保つ
		constexpr float kCenterY = 945.0f;  // 体力バー・必殺ゲージのpos_y=900より少し下

		// 武器レベルのテキストをアイコン右下に重ねるためのオフセット
		constexpr float kLevelTextOffset = kIconSize * 0.32f;

		const float centerX = static_cast<float>(::sys::Window::Get().GetVirtualWidth()) * 0.5f;

		// クールダウン進捗の黒半透明オーバーレイ用の単色板。パーク選択のウィンドウ背景と同じ白テクスチャ
		constexpr const char* kOverlayTexturePath = "Assets/Effect/Texture/White.png";
		// 空きスロット状態の間だけ使うダミーテクスチャ
		constexpr const char* kPlaceholderIconPath = "Assets/Icon/loading.png";

		for (int slot = 0; slot < kSlotCount; ++slot)
		{
			const int col = slot % kColumns;
			const int row = slot / kColumns;
			const float x = centerX + (static_cast<float>(col) - (kColumns - 1) * 0.5f) * kSpacing;
			const float y = kCenterY + (static_cast<float>(row) - (kRows - 1) * 0.5f) * kSpacing;

			// アイコン本体。武器種別に応じたテクスチャはWeaponIconBarSystemが差し替える
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

			// クールダウン進捗オーバーレイ、アイコンの真上に重ねる
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
				sprite.SetLayer(::ecs::SpriteLayer::UI, 4); // アイコンのoffset3より手前
				sprite.IsVisible = false;

				registry.emplace<::ecs::WeaponIconSlotTag>(entity,
					::ecs::WeaponIconSlotTag{ slot, ::ecs::eWeaponIconElement::Overlay, x });
			}

			// 残りクールダウン秒数のテキスト。アイコン中央に重ね、別描画パスのため常に最前面
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

			// 武器レベルのテキスト。アイコン右下に重ね、常時表示でクールダウンとは独立
			{
				const float levelX = x + kLevelTextOffset;
				const float levelY = y + kLevelTextOffset;

				auto entity = manager.CreateEntity();
				auto& text = manager.AddComponent<ecs::TextComponent>(entity);
				text.Text = L"";
				text.X = levelX; // WeaponIconBarSystemがMeasureWidthで中央揃えに書き換える
				text.Y = levelY;
				text.Size = 22.0f; // クールダウン秒数の20.0fより少し大きく、視認性を優先する
				text.Color = { 1.0f, 0.9f, 0.4f, 1.0f }; // 淡い黄色でクールダウン秒数の白と区別する
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

	void GameSceneFactory::CreatePlayerExpBar()
	{
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;

		const float virtualWidth = static_cast<float>(::sys::Window::Get().GetVirtualWidth());

		// 画面上部に横長のバーを配置する。専用画像が無いため白テクスチャを着色してストレッチする
		constexpr float kBarHeight = 16.0f;
		constexpr float kBarY = 8.0f;
		constexpr float kBarLeftMargin = 110.0f; // 左側にレベルテキストの表示余地を空ける
		constexpr float kBarRightMargin = 40.0f;
		const float barWidth = virtualWidth - kBarLeftMargin - kBarRightMargin;

		// 経験値バーの識別色は黄緑。HPバーの赤系・必殺ゲージの青と一目で区別できるようにする
		const ::graphics::Color kExpColor = ::graphics::Color(0.5f, 0.9f, 0.3f, 1.0f);

		constexpr const char* kWhiteTexturePath = "Assets/Effect/Texture/White.png";

		// 背景、空の状態で暗いグレー半透明
		{
			auto entity = manager.CreateEntity();
			auto res = ::graphics::TextureManager::Get().GetOrLoad(kWhiteTexturePath);

			auto& tr = manager.AddComponent<ecs::Transform>(entity);
			tr.Set2DPosition(kBarLeftMargin, kBarY);

			auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, res);
			sprite.Size = { barWidth, kBarHeight };
			sprite.Color = ::graphics::Color(0.0f, 0.0f, 0.0f, 0.5f);
			sprite.SetLayer(::ecs::SpriteLayer::UI, 1);
		}

		// 本体、経験値の充填分。開始時は空で以降PlayerExpBarSystemが充填率で更新する
		{
			auto entity = manager.CreateEntity();
			auto res = ::graphics::TextureManager::Get().GetOrLoad(kWhiteTexturePath);

			auto& tr = manager.AddComponent<ecs::Transform>(entity);
			tr.Set2DPosition(kBarLeftMargin, kBarY);

			auto& sprite = manager.AddComponent<::ecs::Sprite>(entity, res);
			sprite.Size = { barWidth, kBarHeight };
			sprite.Color = kExpColor;
			sprite.FType = ::ecs::FillType::Horizontal;
			sprite.FillAmount = 0.0f;
			sprite.SetLayer(::ecs::SpriteLayer::UI, 2); // 背景のoffset1より手前

			// FillAmountを目標値へ滑らかに追従させる。HPバー・必殺ゲージと同じ手法
			auto& lerp = manager.AddComponent<::ecs::FillAmountLerp>(entity);
			lerp.Target = 0.0f;
			lerp.Speed = 2.0f;

			registry.emplace<::ecs::PlayerExpBarTag>(entity);
		}

		// 現在レベル表示はLv.1形式。バーの左側に配置
		{
			auto entity = manager.CreateEntity();
			auto& text = manager.AddComponent<::ecs::TextComponent>(entity);
			text.Text = L"Lv.1"; // PlayerExpBarSystem が毎フレーム上書きする
			text.X = 20.0f;
			// TextComponent::Yはベースライン基準のため、kBarYのままだと文字が画面外にはみ出るのでバー中心付近まで下げる
			constexpr float kLevelTextBaselineY = 26.0f;
			text.Y = kLevelTextBaselineY;
			text.Size = 28.0f;
			text.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			text.Layer = 10;

			registry.emplace<::ecs::PlayerLevelTextTag>(entity);
		}
	}

}

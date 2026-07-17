#include "apppch.h"
#include "GameSceneFactory.h"

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

// 武器
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/SingleShot/SingleShotWeaponRuntimeComponent.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Orbit/OrbitWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Nova/NovaWeaponRuntimeComponent.h>
#include<system/Player/Weapon/Homing/HomingMissileRuntimeComponent.h>
#include<system/Player/Weapon/ChainLightning/ChainLightningRuntimeComponent.h>
#include<system/Player/Weapon/Meteor/MeteorWeaponRuntimeComponent.h>
#include<system/Player/Weapon/VoidBeam/VoidBeamRuntimeComponent.h>
#include<system/Player/Weapon/BoneSpear/BoneSpearRuntimeComponent.h>
#include<system/Player/Weapon/Cleave/CleaveRuntimeComponent.h>

// 敵
#include<system/Enemy/Move/EnemyChaseComponent.h>
#include<system/Enemy/Attack/EnemyAttackComponent.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>

// UI
#include<system/Player/UI/PlayerUiTag.h>
#include<system/GlowAnimation/GlowAnimationComp.h>

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
			wave.SpawnMarginMin = waveData->SpawnMarginMin;
			wave.SpawnMarginMax = waveData->SpawnMarginMax;
			wave.StatGrowthPerSecond = waveData->StatGrowthPerSecond;
			wave.BossSpawnTime = waveData->BossSpawnTime;
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
		float scale = 20.0f;

		auto entity = manager.CreateEntity();
		auto& tr = manager.AddComponent<::ecs::Transform>(entity);
		tr.SetScale(scale);

		auto& fbx = manager.AddComponent<::ecs::FbxComponent>(entity);
		fbx.Resource = res;
	}

	// プレイヤー
	void GameSceneFactory::CreatePlayer(const CreatePlayerContext& Context)
	{
		// 管理
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;
		auto player_res = ::graphics::FbxResourceManager::Get().Load("Assets/Fbx/Faul/Faul.fbx.bin");

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

		// 必殺技ゲージ（撃破数で蓄積、満タンで"Ultimate"アクションにより発動可能）
		manager.AddComponent<::ecs::PlayerUltimateComponent>(player);

		auto& fill = manager.AddComponent<::ecs::FillAmountLerp>(player);
		fill.Speed = 0.5f;

		// 狙い方向（マウス座標/右スティックでPlayerAimSystemが更新する）
		manager.AddComponent<::ecs::PlayerAimComponent>(player);

		registry.emplace<::ecs::PlayerTag>(player);

		manager.AddComponent<::ecs::WeaponInventoryComponent>(player);

		// 初期武器（いずれもWeaponID=0が対応するCSV(Assets/Data/Weapon/*.csv)のレコードと一致する）。
		// AddWeaponToPlayer()はパーク選択・デバッグ操作からの武器追加とも共通の経路にしてある。
		AddWeaponToPlayer(player, ::ecs::eWeaponType::SingleShot, 0, ::ecs::eWeaponControl::Manual);  // FireBolt（左クリックで発射）
		AddWeaponToPlayer(player, ::ecs::eWeaponType::AreaAttack, 0, ::ecs::eWeaponControl::Manual);  // IceSpike（右クリック"Attack2"で発動）
		// SelfDefense(FrostOrb)は発動トリガーの無い常時稼働の武器のためControlは意味を持たないが、区分上はAutoとする
		AddWeaponToPlayer(player, ::ecs::eWeaponType::SelfDefense, 0, ::ecs::eWeaponControl::Auto);
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
		weaponComp.MaxLevel = 5; // 暫定値。パーク/レベルアップ仕様確定後に見直す
		weaponComp.Level = 1;
		weaponComp.Owner = player;
		weaponComp.Control = control;

		// 武器種別ごとのランタイムコンポーネントを付与する
		switch (type)
		{
		case ::ecs::eWeaponType::SingleShot:
			manager.AddComponent<::ecs::SingleShotWeaponRuntimeComponent>(weapon);
			break;
		case ::ecs::eWeaponType::AreaAttack:
			manager.AddComponent<::ecs::AreaAttackWeaponRuntimeComponent>(weapon);
			break;
		case ::ecs::eWeaponType::SelfDefense:
			manager.AddComponent<::ecs::OrbitWeaponRuntimeComponent>(weapon);
			break;
		case ::ecs::eWeaponType::Nova:
			manager.AddComponent<::ecs::NovaWeaponRuntimeComponent>(weapon);
			break;
		case ::ecs::eWeaponType::Homing:
			manager.AddComponent<::ecs::HomingMissileRuntimeComponent>(weapon);
			break;
		case ::ecs::eWeaponType::Chain:
			manager.AddComponent<::ecs::ChainLightningRuntimeComponent>(weapon);
			break;
		case ::ecs::eWeaponType::Meteor:
			manager.AddComponent<::ecs::MeteorWeaponRuntimeComponent>(weapon);
			break;
		case ::ecs::eWeaponType::VoidBeam:
			manager.AddComponent<::ecs::VoidBeamRuntimeComponent>(weapon);
			break;
		case ::ecs::eWeaponType::BoneSpear:
			manager.AddComponent<::ecs::BoneSpearRuntimeComponent>(weapon);
			break;
		case ::ecs::eWeaponType::Cleave:
			manager.AddComponent<::ecs::CleaveRuntimeComponent>(weapon);
			break;
		}

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
		cam.Far = 1000.0f;
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
		light.ShadowRange = 50.0f;
		light.ShadowTarget = { 0.0f, 0.0f, 0.0f };
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
		// 制限時間のUI
		CreateWaveTimerUI();
	}

	void GameSceneFactory::CreateEnemy(
		const DirectX::XMFLOAT3& position,
		const ecs::EnemyWaveModifier& waveModifier,
		bool isBoss)
	{
		// ボースの強化倍率（暫定値。ボースを複数種類用意する段階になったらデータ化する）
		constexpr float kBossHpMultiplier = 10.0f;
		constexpr float kBossAtkMultiplier = 3.0f;
		constexpr float kBossScaleMultiplier = 2.5f;
		constexpr float kBossExpMultiplier = 20.0f;
		constexpr float kBossGoldMultiplier = 20.0f;

		// 管理
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;
		auto res = ::graphics::FbxResourceManager::Get().Load("Assets/Fbx/Faul/Faul.fbx.bin");

		// 敵の生成
		const float baseScale = 0.2f;
		const float scale = isBoss ? baseScale * kBossScaleMultiplier : baseScale;
		auto enemy = manager.CreateEntity();

		// 座標系
		auto& tr = manager.AddComponent<ecs::Transform>(enemy);
		tr.SetScale(scale);
		tr.SetPosition(position);

		// 物理（コライダーは見た目のスケールに追従しないため、ボースは箱も合わせて拡大する）
		const DirectX::XMFLOAT3 colliderHalfExtent = isBoss
			? DirectX::XMFLOAT3{ 5.0f * kBossScaleMultiplier, 30.0f * kBossScaleMultiplier, 5.0f * kBossScaleMultiplier }
			: DirectX::XMFLOAT3{ 5.0f, 30.0f, 5.0f };
		manager.AddComponent<ecs::ColliderComponent>(enemy, ecs::ColliderComponent::MakeBox(colliderHalfExtent));
		auto& rigid = manager.AddComponent<ecs::RigidBodyComponent>(enemy, ecs::RigidBodyComponent::MakeDynamic());
		rigid.GravityFactor = 0.0f;
		rigid.LinearDamping = 10.0f;

		// モデル（専用モデルが用意されるまではプレイヤーモデルを色違いで代用する）
		auto& fbx = manager.AddComponent<ecs::FbxComponent>(enemy);
		fbx.Resource = res;
		fbx.CustomColor = isBoss
			? DirectX::XMFLOAT4{ 1.0f, 0.2f, 0.2f, 1.0f }
			: DirectX::XMFLOAT4{ 1.0f, 1.0f, 0.5f, 1.0f };

		// 移動（プレイヤーの実移動速度 PlayerMovementComponent::MaxSpeed(50) より少し遅くする）
		auto& chase = manager.AddComponent<::ecs::EnemyChaseComponent>(enemy);
		chase.MoveSpeed = 40.0f;
		auto& rotate = manager.AddComponent<::ecs::RotateToMoveComponent>(enemy);
		rotate.InstantRotate = false;
		manager.AddComponent<::ecs::MoveDirectionComponent>(enemy);

		// ステータス（現在の難易度倍率を適用。ボースはさらに追加倍率をかける）
		auto& status = manager.AddComponent<::ecs::EnemyStatusComponent>(enemy);
		status.WaveMod = waveModifier;
		if (isBoss)
		{
			status.WaveMod.MulMaxHp *= kBossHpMultiplier;
			status.WaveMod.MulAtkPower *= kBossAtkMultiplier;
			status.Base.ExperienceValue *= kBossExpMultiplier;
			status.Base.GoldValue *= kBossGoldMultiplier;
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
  
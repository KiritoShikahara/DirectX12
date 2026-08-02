#include "apppch.h"
#include "GameScene.h"

#include"../macros.h"
#include"Factory/GameSceneFactory.h"

#include<system/RotateToMove/RotateToMoveSystem.h>
#include<scene/Game/State/GameStateSystem.h>

#include<system/CameraFollow/CameraPlayerFollowSystem.h>
#include<system/Light/DirLightFollowSystem.h>

#include<system/Player/UI/PlayerHpBarSystem.h>
#include<system/Player/UI/PlayerUltimateGaugeSystem.h>
#include<system/Player/UI/WeaponIconBarSystem.h>
#include<system/Player/UI/PlayerExpBarSystem.h>
#include<Utility/config/DebugConfig.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>
#include<system/UI/DamageNumber/DamageNumberSystem.h>

#include<system/Player/State/PlayerStateSystem.h>
#include<system/Player/InputSystem/PlayerInputSystem.h>
#include<system/Player/MovementSystem/PlayerMovementSystem.h>
#include<system/Animation/LocomotionAnimationSystem.h>
#include<system/Player/AimSysten/PlayerAimSystem.h>
#include<system/Player/Boundary/PlayerBoundaryClampSystem.h>

#include<system/Player/Weapon/SingleShot/SingleShotWeaponSystem.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackWeaponSystem.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackAutoStrikeSystem.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackHazardSystem.h>
#include<system/Player/Weapon/Orbit/OrbitWeaponSystem.h>
#include<system/Player/Weapon/Nova/NovaWeaponSystem.h>
#include<system/Player/Weapon/Homing/HomingMissileWeaponSystem.h>
#include<system/Player/Weapon/Homing/HomingMissileSteeringSystem.h>
#include<system/Player/Weapon/ChainLightning/ChainLightningWeaponSystem.h>
#include<system/Player/Weapon/Meteor/MeteorWeaponSystem.h>
#include<system/Player/Weapon/VoidBeam/VoidBeamWeaponSystem.h>
#include<system/Player/Weapon/BoneSpear/BoneSpearWeaponSystem.h>
#include<system/Player/Weapon/Cleave/CleaveWeaponSystem.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeWeaponSystem.h>
#include<system/Player/Weapon/Ricochet/RicochetWeaponSystem.h>
#include<system/Player/Ultimate/PlayerUltimateSystem.h>
#include<system/Player/Weapon/Projectile/ProjectileMovementSystem.h>
#include<system/Player/Weapon/Projectile/ProjectileCollisionSystem.h>
#include<system/Effect/TemporaryLifetimeSystem.h>

#include<system/Enemy/Move/EnemyChaseSystem.h>
#include<system/Enemy/Death/EnemyDeathSystem.h>
#include<system/Enemy/Knockback/EnemyKnockbackSystem.h>
#include<system/Enemy/Status/EnemySlowStatusSystem.h>

#include<system/Player/Perk/PerkSelectSystem.h>
#include<system/Options/OptionsMenuSystem.h>

#include<Scene/Game/Result/ResultSystem.h>

#include<Scene/Game/Wave/WaveComponent.h>
#include<Scene/Game/Wave/EnemySpawnSystem.h>
#include<Scene/Game/Wave/WaveTimerUiSystem.h>

#include<system/Damage/PlayerContactDamage/PlayerContactDamageSystem.h>
#include<system/Player/Status/PlayerRegenSystem.h>

#include<system/Effect/EffectSpawnUtility.h>

#include<Data/Enemy/EnemyData.h>
#include<Data/Enemy/BossData.h>
#include<Data/Effect/EffectAssetData.h>
#include<Data/Weapon/SingleShotWeaponData.h>
#include<Data/Weapon/AreaAttackWeaponData.h>
#include<Data/Weapon/OrbitWeaponData.h>
#include<Data/Weapon/NovaWeaponData.h>
#include<Data/Weapon/HomingMissileWeaponData.h>
#include<Data/Weapon/ChainLightningWeaponData.h>
#include<Data/Weapon/MeteorWeaponData.h>
#include<Data/Weapon/VoidBeamWeaponData.h>
#include<Data/Weapon/BoneSpearWeaponData.h>
#include<Data/Weapon/CleaveWeaponData.h>
#include<Data/Weapon/FlickerStrikeWeaponData.h>
#include<Data/Weapon/RicochetWeaponData.h>
#include<Data/Ultimate/UltimateData.h>
#include<Data/StatUpgrade/StatUpgradeData.h>
#include<Data/Save/PlayerSaveData.h>
#include<Data/Wave/WaveData.h>
#include<Data/Perk/PerkData.h>

namespace scene
{
	GameScene::GameScene(uint32_t SpellID)
		:mSpellID(SpellID)
	{
	}

	void GameScene::Initialize()
	{

		LoadData();

		CreateUserSystem();

		LoadResource();

		PreloadWeaponEffects();

		CreateEntitys();

#if DEV_TOOL_ENABLED
		this->DebugInitialize();


#endif // _DEBUG

		DEBUG_LOG(::sys::eLogLevel::Log, "Game Scene.");
	}

	void GameScene::Finalize()
	{

#if DEV_TOOL_ENABLED
			this->DebugFinalize();


#endif // _DEBUG
	}

	void GameScene::LoadData()
	{
		auto id = mSpellID;

		// DataRegistryはプロセス全体で1つのシングルトンのため、GameSceneの再初期化時に二重登録でassertに落ちないようガードする
		auto& dataRegistry = data::DataRegistry::Get();
		if (!dataRegistry.IsRegistered<data::EnemyData>())
		{
			dataRegistry.Register<data::EnemyData>("Assets/Data/Enemy/EnemyData.csv");
		}
		if (!dataRegistry.IsRegistered<data::BossData>())
		{
			dataRegistry.Register<data::BossData>("Assets/Data/Enemy/BossData.csv");
		}
		// エフェクト素材ID→パスの解決テーブル。武器データより先にロードされている必要があるためここへ置く
		if (!dataRegistry.IsRegistered<data::EffectAssetData>())
		{
			dataRegistry.Register<data::EffectAssetData>("Assets/Data/Effect/EffectAssetData.csv");
		}
		if (!dataRegistry.IsRegistered<data::SingleShotWeaponData>())
		{
			dataRegistry.Register<data::SingleShotWeaponData>("Assets/Data/Weapon/SingleShotWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::AreaAttackWeaponData>())
		{
			dataRegistry.Register<data::AreaAttackWeaponData>("Assets/Data/Weapon/AreaAttackWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::OrbitWeaponData>())
		{
			dataRegistry.Register<data::OrbitWeaponData>("Assets/Data/Weapon/OrbitWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::NovaWeaponData>())
		{
			dataRegistry.Register<data::NovaWeaponData>("Assets/Data/Weapon/NovaWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::HomingMissileWeaponData>())
		{
			dataRegistry.Register<data::HomingMissileWeaponData>("Assets/Data/Weapon/HomingMissileWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::ChainLightningWeaponData>())
		{
			dataRegistry.Register<data::ChainLightningWeaponData>("Assets/Data/Weapon/ChainLightningWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::MeteorWeaponData>())
		{
			dataRegistry.Register<data::MeteorWeaponData>("Assets/Data/Weapon/MeteorWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::VoidBeamWeaponData>())
		{
			dataRegistry.Register<data::VoidBeamWeaponData>("Assets/Data/Weapon/VoidBeamWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::BoneSpearWeaponData>())
		{
			dataRegistry.Register<data::BoneSpearWeaponData>("Assets/Data/Weapon/BoneSpearWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::CleaveWeaponData>())
		{
			dataRegistry.Register<data::CleaveWeaponData>("Assets/Data/Weapon/CleaveWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::FlickerStrikeWeaponData>())
		{
			dataRegistry.Register<data::FlickerStrikeWeaponData>("Assets/Data/Weapon/FlickerStrikeWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::RicochetWeaponData>())
		{
			dataRegistry.Register<data::RicochetWeaponData>("Assets/Data/Weapon/RicochetWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::UltimateData>())
		{
			dataRegistry.Register<data::UltimateData>("Assets/Data/Ultimate/UltimateData.csv");
		}
		if (!dataRegistry.IsRegistered<data::WaveData>())
		{
			dataRegistry.Register<data::WaveData>("Assets/Data/Wave/WaveData.csv");
		}
		if (!dataRegistry.IsRegistered<data::StatUpgradeData>())
		{
			dataRegistry.Register<data::StatUpgradeData>("Assets/Data/StatUpgrade/StatUpgradeData.csv");
		}
		if (!dataRegistry.IsRegistered<data::PerkData>())
		{
			dataRegistry.Register<data::PerkData>("Assets/Data/Perk/PerkData.csv");
		}
		dataRegistry.LoadAll();

		// プレイヤーの永続進行状況はCSV/DBのマスタデータと別系統のJSON永続化のため、DataRegistryとは別に読み込む
		data::EnsurePlayerSaveDataLoaded();
	}

	void GameScene::LoadResource()
	{
		auto& manager = ::graphics::FbxResourceManager::Get();

		// Field
		manager.Load("Assets/Fbx/Field/Field.fbx.bin");

		// Player
		auto playerPath = "Assets/Fbx/Faul/Faul.fbx.bin";
		manager.Load(playerPath);

	}

	void GameScene::PreloadWeaponEffects()
	{
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::SingleShotWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::AreaAttackWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::OrbitWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::NovaWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::HomingMissileWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::ChainLightningWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::MeteorWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::VoidBeamWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::BoneSpearWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::CleaveWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::FlickerStrikeWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::RicochetWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::UltimateData).GetAll());

		// マスタデータに属さない固定演出。EnemyDeathSystem/PerkSelectSystemのパス文字列と一致させること
		ecs::effectutil::PreloadEffect("Assets/Effect/AttackHit.efk");
		ecs::effectutil::PreloadEffect("Assets/Effect/Herald.efk");
	}

	void GameScene::CreateUserSystem()
	{
		auto& manager = ::ecs::ComponentSystemManager::Get();
		manager.AddUserSystem<::ecs::PlayerInputSystem>(::ecs::eUpdatePhase::PreUpdate);
		manager.AddUserSystem<::sys::PlayerAimSystem>(::ecs::eUpdatePhase::PreUpdate);
		manager.AddUserSystem<::ecs::PlayerStateSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerMovementSystem>(::ecs::eUpdatePhase::Update);
		// スロウ状態の期限切れ解除は速度を参照するEnemyChaseSystemより前に置く
		manager.AddUserSystem<::ecs::EnemySlowStatusSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::EnemyChaseSystem>(::ecs::eUpdatePhase::Update);
		// 移動状態に応じてIdle/Runへ切り替える。PlayerMovementSystem/EnemyChaseSystemの後でFbxAnimSystemより前に置く
		manager.AddUserSystem<::ecs::LocomotionAnimationSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::EnemySpawnSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::WaveTimerUiSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerHpBarSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerUltimateGaugeSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::WeaponIconBarSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerExpBarSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerContactDamageSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerRegenSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::SingleShotWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::AreaAttackWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::AreaAttackAutoStrikeSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::AreaAttackHazardSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::OrbitWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::NovaWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::HomingMissileWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::HomingMissileSteeringSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::ChainLightningWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::MeteorWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::VoidBeamWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::BoneSpearWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::CleaveWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::FlickerStrikeWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::RicochetWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::ProjectileCollisionSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::ProjectileMovementSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::TemporaryLifetimeSystem>(::ecs::eUpdatePhase::Update);
		// ノックバック処理はEnemyChaseSystemより前に実行し、同一フレームでMoveVelocityの上書き合戦を防ぐ
		manager.AddUserSystem<::ecs::EnemyKnockbackSystem>(::ecs::eUpdatePhase::Update);
		// ダメージ計算のUpdateが終わった後にHP0の敵をまとめて破棄する
		manager.AddUserSystem<::ecs::EnemyDeathSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::RotateToMoveSystem>(::ecs::eUpdatePhase::PostUpdate);
		// PlayerUltimateSystemはCameraOverrideComponentへ位置リクエストを書くだけなので、消費するCameraPlayerFollowSystemより前段に置く
		manager.AddUserSystem<::ecs::PlayerUltimateSystem>(::ecs::eUpdatePhase::PostUpdate);
		// このフレームのテレポートを境界内へクランプしてからCameraPlayerFollowSystemに位置を読ませる
		manager.AddUserSystem<::ecs::PlayerBoundaryClampSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::CameraPlayerFollowSystem>(::ecs::eUpdatePhase::PostUpdate);
		// LightSystem::Updateより前にShadowTargetを確定させる必要があるためPostUpdateの中で登録する
		manager.AddUserSystem<::ecs::DirLightFollowSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::DamageNumberSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::sys::GameStateSystem>(::ecs::eUpdatePhase::PostUpdate);
		// GameStateSystemの後段に置くことで、PerkSelect/Resultへ遷移した同一フレームでUIを生成できる
		manager.AddUserSystem<::ecs::PerkSelectSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::ResultSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::SpriteGlowSystem>(::ecs::eUpdatePhase::PostUpdate);
		// 設定メニューはTimeScaleを0にするため、他システムが状態を更新し終えた後段に置く
		manager.AddUserSystem<::ecs::OptionsMenuSystem>(::ecs::eUpdatePhase::PostUpdate);
	}

	void GameScene::CreateEntitys()
	{
		::ecs::GameSceneFactory::CreateStateController();
		::ecs::GameSceneFactory::CreateBGM();
		::ecs::GameSceneFactory::CreateDirLight();
		::ecs::GameSceneFactory::CreateGround();
		// フィールド外へ出られないようにする見えない境界壁
		::ecs::GameSceneFactory::CreateFieldBoundary();
		// Skybox。フィールド外側が虚無に見えないようにする
		::ecs::GameSceneFactory::CreateSkybox();
		::ecs::GameSceneFactory::CreatePlayer(::ecs::CreatePlayerContext{ mSpellID });
		::ecs::GameSceneFactory::CreateCamera();
		::ecs::GameSceneFactory::CreateUI();

		// 敵はEnemySpawnSystemが継続的に生成するため、ここでの固定生成は行わない
	}

	void GameScene::DebugInitialize()
	{
		mEnemyStatusDebugPanel = std::make_unique<debug::EnemyStatusDebugPanel>();
		mGameStatusDebugPanel = std::make_unique<debug::GameStatusDebugPanel>();
		mWaveDebugPanel = std::make_unique<debug::WaveDebugPanel>();
		mPerkDebugPanel = std::make_unique<debug::PerkDebugPanel>();
		mWeaponMasterDataDebugPanel = std::make_unique<debug::WeaponMasterDataDebugPanel>();
		mWeaponInventoryDebugPanel = std::make_unique<debug::WeaponInventoryDebugPanel>();
		mUltimateDebugPanel = std::make_unique<debug::UltimateDebugPanel>();
		mPlayerSaveDebugPanel = std::make_unique<debug::PlayerSaveDebugPanel>("GameScene_PlayerSaveDebug");
		mStatUpgradeDebugPanel = std::make_unique<debug::StatUpgradeDebugPanel>();
		mEffectAssetDebugPanel = std::make_unique<debug::EffectAssetDebugPanel>();
		mBossDebugPanel = std::make_unique<debug::BossDebugPanel>();
	}

	void GameScene::DebugFinalize()
	{
		mEnemyStatusDebugPanel.reset();
		mGameStatusDebugPanel.reset();
		mWaveDebugPanel.reset();
		mPerkDebugPanel.reset();
		mWeaponMasterDataDebugPanel.reset();
		mWeaponInventoryDebugPanel.reset();
		mUltimateDebugPanel.reset();
		mPlayerSaveDebugPanel.reset();
		mStatUpgradeDebugPanel.reset();
		mEffectAssetDebugPanel.reset();
		mBossDebugPanel.reset();
	}


	REGISTER_SCENE_AS(GameScene, GAME_SCENE_NAME);

}



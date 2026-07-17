#include "apppch.h"
#include "GameScene.h"

#include"../macros.h"
#include"Factory/GameSceneFactory.h"

#include<system/RotateToMove/RotateToMoveSystem.h>
#include<scene/Game/State/GameStateSystem.h>

// カメラ
#include<system/CameraFollow/CameraPlayerFollowSystem.h>

// UI
#include<system/Player/UI/PlayerHpBarSystem.h>
#include<system/GlowAnimation/SpriteGlowSystem.h>
#include<system/UI/DamageNumber/DamageNumberSystem.h>

// プレイヤー
#include<system/Player/State/PlayerStateSystem.h>
#include<system/Player/InputSystem/PlayerInputSystem.h>
#include<system/Player/MovementSystem/PlayerMovementSystem.h>
#include<system/Player/AimSysten/PlayerAimSystem.h>

// 武器
#include<system/Player/Weapon/SingleShot/SingleShotWeaponSystem.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackWeaponSystem.h>
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
#include<system/Player/Ultimate/PlayerUltimateSystem.h>
#include<system/Player/Weapon/Projectile/ProjectileMovementSystem.h>
#include<system/Player/Weapon/Projectile/ProjectileCollisionSystem.h>
#include<system/Effect/TemporaryLifetimeSystem.h>

// 敵
#include<system/Enemy/Move/EnemyChaseSystem.h>
#include<system/Enemy/Death/EnemyDeathSystem.h>
#include<system/Enemy/Knockback/EnemyKnockbackSystem.h>

// パーク
#include<system/Player/Perk/PerkSelectSystem.h>

// リザルト
#include<Scene/Game/Result/ResultSystem.h>

// ウェーブ
#include<Scene/Game/Wave/WaveComponent.h>
#include<Scene/Game/Wave/EnemySpawnSystem.h>
#include<Scene/Game/Wave/WaveTimerUiSystem.h>

// ダメージ
#include<system/Damage/PlayerContactDamage/PlayerContactDamageSystem.h>

// データ
#include<Data/Enemy/EnemyData.h>
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
#include<Data/Ultimate/UltimateData.h>
#include<Data/StatUpgrade/StatUpgradeData.h>
#include<Data/Save/PlayerSaveData.h>
#include<Data/Wave/WaveData.h>

// デバッグ

namespace scene
{
	GameScene::GameScene(uint32_t SpellID)
		:mSpellID(SpellID)
	{
	}

	void GameScene::Initialize()
	{

		// データ
		LoadData();

		// システム
		CreateUserSystem();

		// リソース
		LoadResource();

		// エンティティ生成
		CreateEntitys();

#ifdef _DEBUG
		// デバッグ
		this->DebugInitialize();


#endif // _DEBUG

		DEBUG_LOG(::sys::eLogLevel::Log, "Game Scene.");
	}

	void GameScene::Finalize()
	{
		
#ifdef _DEBUG
			// デバッグ
			this->DebugFinalize();


#endif // _DEBUG
	}

	void GameScene::LoadData()
	{
		auto id = mSpellID;

		// データ読み込み
		// DataRegistry はプロセス全体で1つのシングルトンのため、
		// GameScene が再初期化される(エディタの Stop によるシーン再構築等)場合、
		// 二重登録で RegistryBase の assert に落ちないようにガードする。
		auto& dataRegistry = data::DataRegistry::Get();
		if (!dataRegistry.IsRegistered<data::EnemyData>())
		{
			dataRegistry.Register<data::EnemyData>("Assets/Data/Enemy/EnemyData.csv");
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
		dataRegistry.LoadAll();

		// プレイヤーの永続的な進行状況(ゴールド・ステータス強化レベル)。CSV/DBのマスタデータとは
		// 別系統(ConfigManager<T>によるJSON永続化)のため、DataRegistryとは別に読み込む
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
		/// アニメーション

		// Enemy

		// Texture

	}

	void GameScene::CreateUserSystem()
	{
		auto& manager = ::ecs::ComponentSystemManager::Get();
		manager.AddUserSystem<::ecs::PlayerInputSystem>(::ecs::eUpdatePhase::PreUpdate);
		manager.AddUserSystem<::sys::PlayerAimSystem>(::ecs::eUpdatePhase::PreUpdate);
		manager.AddUserSystem<::ecs::PlayerStateSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerMovementSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::EnemyChaseSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::EnemySpawnSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::WaveTimerUiSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerHpBarSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerContactDamageSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::SingleShotWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::AreaAttackWeaponSystem>(::ecs::eUpdatePhase::Update);
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
		manager.AddUserSystem<::ecs::ProjectileCollisionSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::ProjectileMovementSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::TemporaryLifetimeSystem>(::ecs::eUpdatePhase::Update);
		// ノックバック処理(Cleave)はEnemyChaseSystemより前に実行し、
		// 同一フレーム内でMoveVelocityの上書き合戦にならないようにする
		manager.AddUserSystem<::ecs::EnemyKnockbackSystem>(::ecs::eUpdatePhase::Update);
		// ダメージ計算(Update)が終わった後にHP0の敵をまとめて破棄する
		manager.AddUserSystem<::ecs::EnemyDeathSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::RotateToMoveSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::CameraPlayerFollowSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::DamageNumberSystem>(::ecs::eUpdatePhase::PostUpdate);
		// 必殺技演出中はCameraPlayerFollowSystemの通常追従をこの後で上書きする必要があるため、
		// 必ずCameraPlayerFollowSystemの後段に置く
		manager.AddUserSystem<::ecs::PlayerUltimateSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::sys::GameStateSystem>(::ecs::eUpdatePhase::PostUpdate);
		// GameStateSystemの後段に置くことで、PerkSelect/Resultへ遷移した同一フレームでUIを生成できる
		manager.AddUserSystem<::ecs::PerkSelectSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::ResultSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::SpriteGlowSystem>(::ecs::eUpdatePhase::PostUpdate);
	}

	void GameScene::CreateEntitys()
	{
		// 状態
		::ecs::GameSceneFactory::CreateStateController();
		// BGM
		::ecs::GameSceneFactory::CreateBGM();
		// ディレクションライト
		::ecs::GameSceneFactory::CreateDirLight();
		// 地面
		::ecs::GameSceneFactory::CreateGround();
		// プレイヤー
		::ecs::GameSceneFactory::CreatePlayer(::ecs::CreatePlayerContext{ mSpellID });
		// カメラ
		::ecs::GameSceneFactory::CreateCamera();
		// UI
		::ecs::GameSceneFactory::CreateUI();

		// 敵はEnemySpawnSystemが継続的に生成するため、ここでの固定生成は行わない
	}

	void GameScene::DebugInitialize()
	{
		mEnemyStatusDebugPanel = std::make_unique<debug::EnemyStatusDebugPanel>();
		mGameStatusDebugPanel = std::make_unique<debug::GameStatusDebugPanel>();
		mWaveDebugPanel = std::make_unique<debug::WaveDebugPanel>();
		mSingleShotWeaponDebugPanel = std::make_unique<debug::SingleShotWeaponDebugPanel>();
		mWeaponInventoryDebugPanel = std::make_unique<debug::WeaponInventoryDebugPanel>();
		mUltimateDebugPanel = std::make_unique<debug::UltimateDebugPanel>();
		mPlayerSaveDebugPanel = std::make_unique<debug::PlayerSaveDebugPanel>("GameScene_PlayerSaveDebug");
	}

	void GameScene::DebugFinalize()
	{
		mEnemyStatusDebugPanel.reset();
		mGameStatusDebugPanel.reset();
		mWaveDebugPanel.reset();
		mSingleShotWeaponDebugPanel.reset();
		mWeaponInventoryDebugPanel.reset();
		mUltimateDebugPanel.reset();
		mPlayerSaveDebugPanel.reset();
	}


	REGISTER_SCENE_AS(GameScene, GAME_SCENE_NAME);

}



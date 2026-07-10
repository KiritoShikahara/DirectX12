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

// プレイヤー
#include<system/Player/State/PlayerStateSystem.h>
#include<system/Player/InputSystem/PlayerInputSystem.h>
#include<system/Player/MovementSystem/PlayerMovementSystem.h>

// 敵
#include<system/Enemy/Move/EnemyChaseSystem.h>

// ダメージ
#include<system/Damage/PlayerContactDamage/PlayerContactDamageSystem.h>

// データ
#include<Data/Enemy/EnemyData.h>

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
		data::DataRegistry::Get().Register<data::EnemyData>("Assets/Data/Enemy/EnemyData.csv");
		data::DataRegistry::Get().LoadAll();

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
		manager.AddUserSystem<::ecs::PlayerStateSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerMovementSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::EnemyChaseSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerHpBarSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerContactDamageSystem>(::ecs::eUpdatePhase::Update);	
		manager.AddUserSystem<::ecs::RotateToMoveSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::CameraPlayerFollowSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::sys::GameStateSystem>(::ecs::eUpdatePhase::PostUpdate);
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

		// 敵
		::ecs::GameSceneFactory::CreateEnemy();
	}

	void GameScene::DebugInitialize()
	{
		mEnemyStatusDebugPanel = std::make_unique<debug::EnemyStatusDebugPanel>();
	}

	void GameScene::DebugFinalize()
	{
		mEnemyStatusDebugPanel.reset();
	}


	REGISTER_SCENE_AS(GameScene, GAME_SCENE_NAME);

}



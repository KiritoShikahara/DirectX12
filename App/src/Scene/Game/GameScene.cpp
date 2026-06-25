#include "apppch.h"
#include "GameScene.h"

#include"../macros.h"
#include"Factory/GameSceneFactory.h"

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

		// エンティティ生成
		CreateEntitys();

		DEBUG_LOG(::sys::eLogLevel::Log, "Game Scene.");
	}

	void GameScene::Finalize()
	{
	}

	void GameScene::LoadData()
	{
		auto id = mSpellID;
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

	void GameScene::CreateEntitys()
	{
		// BGM
		::ecs::GameSceneFactory::CreateBGM();
		// 状態
		::ecs::GameSceneFactory::CreateStateObject();
		// ディレクションライト
		::ecs::GameSceneFactory::CreateDirLight();
		// 地面
		::ecs::GameSceneFactory::CreateGround();
		// プレイヤー
		::ecs::GameSceneFactory::CreatePlayer(::ecs::CreatePlayerContext{ mSpellID });
		// カメラ
		::ecs::GameSceneFactory::CreateCamera();
	}


	REGISTER_SCENE_AS(GameScene, GAME_SCENE_NAME);

}



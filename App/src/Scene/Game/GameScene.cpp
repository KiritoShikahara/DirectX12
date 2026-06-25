#include "apppch.h"
#include "GameScene.h"

#include"../macros.h"

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
		// BGM
		CreateBGM();
		// 状態
		CreateStateObject();
		// 地面
		CreateGround();
		// プレイヤー
		CreatePlayer();
		// カメラ
		CreateCamera();

		DEBUG_LOG(::sys::eLogLevel::Log, "Game Scene.");
	}

	void GameScene::Finalize()
	{
	}

	void GameScene::LoadData()
	{
		auto id = mSpellID;
	}

	void GameScene::CreateBGM()
	{

	}
	void GameScene::CreateStateObject()
	{

	}
	void GameScene::CreateGround()
	{

	}
	void GameScene::CreatePlayer()
	{

	}
	void GameScene::CreateCamera()
	{

	}

	REGISTER_SCENE_AS(GameScene, GAME_SCENE_NAME);

}



#include "apppch.h"
#include "MenuScene.h"


namespace scene
{
	void MenuScene::Initialize()
	{
		DEBUG_LOG(::sys::eLogLevel::Log, "Menu Scene.");
	}

	void MenuScene::Finalize()
	{
	}

	REGISTER_SCENE_AS(MenuScene, "Menu");

}

#include<Windows.h>

#if _DEBUG
#define _CRTDBG_MAP_ALLOC 
#include <crtdbg.h>
#endif // _DEBUG

#include<system/Engine/Engine.h>


#include<Scene/Test/TestScene.h>
#include<Scene/Title/TitleScene.h>
#include<system/Scene/Factory/SceneFactory.h>

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{

	sys::SceneFactory::Get().SetDefaultSceneName("Title");

	// システム初期化
	auto& engine = sys::Engine::Get();
	if (engine.Initialize() == false)
	{
		return -1;
	}

	// ループ
	while (engine.Run())
	{
	}

	// ファイナライズ
	engine.Finalize();

	return 0;
}
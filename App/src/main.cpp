#include<Windows.h>

#if _DEBUG
#define _CRTDBG_MAP_ALLOC 
#include <crtdbg.h>
#endif // _DEBUG

#include<stdexcept>
#include<system/Engine/Engine.h>
#include<system/Engine/EngineContext.h>

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// パラメーター初期化

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

	return 0;
}
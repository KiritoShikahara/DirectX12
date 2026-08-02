#include <Windows.h>
#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // _DEBUG
#include <system/Engine/Engine.h>
#include <Scene/Test/TestScene.h>
#include <Scene/Title/TitleScene.h>
#include <system/Scene/Factory/SceneFactory.h>
#include <Scene/Game/Debug/GameDebugSettings.h>
#include <Data/Settings/GameSettingsData.h>
#include <chrono>
#include "macros.h"

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	sys::SceneFactory::Get().SetDefaultSceneName(START_SCENE_NAME);

	auto& debugSettings = debug::GameDebugSettings::Get();
	debugSettings.ParseCommandLine(lpCmdLine);

	// システム初期化
	auto& engine = sys::Engine::Get();
	if (engine.Initialize() == false)
	{
		return -1;
	}

	debugSettings.Initialize();

	// 音量などのユーザー設定を読み込んでAudioManagerへ引き渡す。
	data::EnsureGameSettingsLoaded();

	const float autoExitSeconds = debugSettings.GetAutoExitSeconds();
	const auto  startTime = std::chrono::steady_clock::now();

	// ループ
	while (engine.Run())
	{
		if (autoExitSeconds > 0.0f)
		{
			const auto elapsed = std::chrono::duration<float>(
				std::chrono::steady_clock::now() - startTime).count();
			if (elapsed >= autoExitSeconds) break;
		}
	}

	debugSettings.Finalize();

	// ファイナライズ
	engine.Finalize();

	return 0;
}
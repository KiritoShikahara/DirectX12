#include<Windows.h>

#if _DEBUG
#define _CRTDBG_MAP_ALLOC 
#include <crtdbg.h>
#endif // _DEBUG

#include<system/Engine/Engine.h>


#include<Scene/Test/TestScene.h>
#include<Scene/Title/TitleScene.h>
#include<system/Scene/Factory/SceneFactory.h>

#include<Scene/Game/Debug/GameDebugSettings.h>
#include<Data/Settings/GameSettingsData.h>
#include<chrono>

#include"macros.h"

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	sys::SceneFactory::Get().SetDefaultSceneName(START_SCENE_NAME);

	// 諤ｧ閭ｽ險域ｸｬ繧定・蜍募喧縺吶ｋ髫帙；UI謫堺ｽ懊↑縺励〒譚｡莉ｶ繧貞・迴ｾ縺ｧ縺阪ｋ繧医≧縺ｫ縺吶ｋ縺溘ａ縲・
	// 繝・ヰ繝・げ險ｭ螳壹・繧ｨ繝ｳ繧ｸ繝ｳ蛻晄悄蛹悶ｈ繧雁燕縺ｫ繧ｳ繝槭Φ繝峨Λ繧､繝ｳ縺九ｉ隱ｭ縺ｿ霎ｼ繧
	auto& debugSettings = debug::GameDebugSettings::Get();
	debugSettings.ParseCommandLine(lpCmdLine);

	// 繧ｷ繧ｹ繝・Β蛻晄悄蛹・
	auto& engine = sys::Engine::Get();
	if (engine.Initialize() == false)
	{
		return -1;
	}

	debugSettings.Initialize();

	// 髻ｳ驥上↑縺ｩ縺ｮ繝ｦ繝ｼ繧ｶ繝ｼ險ｭ螳壹ｒ隱ｭ縺ｿ霎ｼ繧薙〒AudioManager縺ｸ蜿肴丐縺吶ｋ縲・
	// 繧ｿ繧､繝医Ν繧貞性繧蜈ｨ繧ｷ繝ｼ繝ｳ縺ｧ譛牙柑縺ｫ縺吶ｋ縺溘ａ縲√す繝ｼ繝ｳ縺ｧ縺ｯ縺ｪ縺上％縺薙〒荳蠎ｦ縺縺題｡後≧
	data::EnsureGameSettingsLoaded();

	// --autoexit=SECONDS 謖・ｮ壽凾縺ｫ菴ｿ縺・幕蟋区凾蛻ｻ(閾ｪ蜍戊ｨ域ｸｬ繧剃ｸ螳壽凾髢薙〒謇薙■蛻・ｋ縺溘ａ)
	const float autoExitSeconds = debugSettings.GetAutoExitSeconds();
	const auto  startTime = std::chrono::steady_clock::now();

	// 繝ｫ繝ｼ繝・
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

	// 繝輔ぃ繧､繝翫Λ繧､繧ｺ
	engine.Finalize();

	return 0;
}
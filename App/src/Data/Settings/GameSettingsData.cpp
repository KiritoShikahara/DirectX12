#include "apppch.h"
#include "GameSettingsData.h"

#include <audio/Manager/AudioManager.h>

namespace data
{
	void EnsureGameSettingsLoaded()
	{
		auto& configReg = ::data::ConfigRegistry::Get();
		if (configReg.IsRegistered<GameSettingsData>()) return;

		configReg.Register<GameSettingsData>("Assets/Bin/Save/game_settings.json");
		configReg.GetManager<GameSettingsData>().Load();

		// ファイルが無い初回起動時は既定値が入るため、いずれの場合も反映してよい
		ApplyGameSettings();
	}

	void ApplyGameSettings()
	{
		auto& configReg = ::data::ConfigRegistry::Get();
		if (!configReg.IsRegistered<GameSettingsData>()) return;

		const auto& settings = configReg.GetManager<GameSettingsData>().Get();

		auto& audio = ::audio::AudioManager::Get();
		audio.SetMasterVolume(settings.MasterVolume);
		audio.SetBgmVolume(settings.BgmVolume);
		audio.SetSeVolume(settings.SeVolume);
	}

	void SaveGameSettings()
	{
		auto& configReg = ::data::ConfigRegistry::Get();
		if (!configReg.IsRegistered<GameSettingsData>()) return;

		configReg.GetManager<GameSettingsData>().Save();
	}
}

#pragma once

#include <Data/Storage/Reflection.h>
#include <Data/Storage/Registry/ConfigRegistry.h>

namespace data
{
	/// <summary>プレイヤーが変更できるゲーム設定（音量など）。ConfigManager&lt;T&gt;によりJSONへ永続化する</summary>
	struct GameSettingsData
	{
		// 音量は 0.0(無音) 〜 1.0(最大)。
		// 実際の再生音量は Master * (BgmまたはSe) で決まる(sys::AudioManager参照)
		float MasterVolume = 1.0f;
		float BgmVolume = 0.8f;
		float SeVolume = 0.8f;

		REFLECT_BEGIN(GameSettingsData, "game_settings")
			REFLECT_FIELD_FLOAT(MasterVolume)
			REFLECT_FIELD_FLOAT(BgmVolume)
			REFLECT_FIELD_FLOAT(SeVolume)
			REFLECT_END()
	};

	/// <summary>GameSettingsDataをConfigRegistryへ未登録なら登録し、ディスクから読み込んでAudioManagerへ反映する</summary>
	void EnsureGameSettingsLoaded();

	/// <summary>現在の設定値をAudioManagerへ反映する</summary>
	void ApplyGameSettings();

	/// <summary>現在の設定値をJSONへ保存する</summary>
	void SaveGameSettings();
}

REFLECT_REGISTER(data::GameSettingsData);
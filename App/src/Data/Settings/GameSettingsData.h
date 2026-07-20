#pragma once

#include<Data/Storage/Reflection.h>
#include<Data/Storage/Registry/ConfigRegistry.h>

namespace data
{
	/// <summary>
	/// プレイヤーが変更できるゲーム設定（音量など）。
	/// アプリを再起動しても引き継がれる（ConfigManager&lt;T&gt;によるJSON永続化。
	/// PlayerSaveDataと同じ仕組み）。
	///
	/// ゲームの進行状況ではなく「環境設定」であるため、セーブデータ(player_save.json)とは
	/// 別ファイルに分ける。セーブデータを消しても音量設定は残る、という挙動が自然なため。
	///
	/// 将来的に画質設定を追加する場合もここへフィールドを足す
	/// (DBは介さないためスキーマ移行は不要。読み込み時に未知のキーは既定値になる)。
	/// </summary>
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

	/// <summary>
	/// GameSettingsDataをConfigRegistryへ未登録なら登録し、ディスクから読み込んで
	/// AudioManagerへ反映する。既に登録済みなら何もしない
	/// (プロセス全体で1つの状態を共有し、二重ロードで未保存の変更を失わないため)。
	/// </summary>
	void EnsureGameSettingsLoaded();

	/// <summary>
	/// 現在の設定値をAudioManagerへ反映する。
	/// 設定を変更した直後に呼ぶこと(保存はSaveGameSettings)。
	/// </summary>
	void ApplyGameSettings();

	/// <summary>現在の設定値をJSONへ保存する</summary>
	void SaveGameSettings();
}

REFLECT_REGISTER(data::GameSettingsData);

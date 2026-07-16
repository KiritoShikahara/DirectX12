#pragma once

#include<Data/Storage/Reflection.h>
#include<Data/Storage/Registry/ConfigRegistry.h>

namespace data
{
	/// <summary>
	/// プレイヤーの永続的な進行状況（所持ゴールド・ステータス恒久強化レベル）。
	/// アプリを再起動しても引き継がれる（ConfigManager&lt;T&gt;によるJSON自動保存/読込、
	/// WindowConfigと同じ仕組み）。CSVで管理するマスタデータ(DataManager&lt;T&gt;)とは異なり、
	/// プレイの結果によって書き換わるセーブデータそのものであるため、Debug/Releaseで
	/// 挙動を分けず常に同じファイル(Assets/Bin/Save/player_save.json)を読み書きする。
	/// </summary>
	struct PlayerSaveData
	{
		/// <summary>所持ゴールド。INGameで敵を倒すことで加算される(EnemyDeathSystem参照)</summary>
		int Gold = 0;

		// 各ステータスの強化レベル(0始まり)。上限はStatUpgradeData::MaxLevel(CSV/DB)で管理する。
		int MaxHpLevel = 0;
		int AtkPowerLevel = 0;
		int DefenseLevel = 0;
		int CooldownRateLevel = 0;

		REFLECT_BEGIN(PlayerSaveData, "player_save")
			REFLECT_FIELD_INT(Gold)
			REFLECT_FIELD_INT(MaxHpLevel)
			REFLECT_FIELD_INT(AtkPowerLevel)
			REFLECT_FIELD_INT(DefenseLevel)
			REFLECT_FIELD_INT(CooldownRateLevel)
		REFLECT_END()
	};

	/// <summary>
	/// PlayerSaveDataをConfigRegistryへ未登録なら登録し、ディスクから読み込む。
	/// 既に登録済みなら何もしない(プロセス全体で1つの状態を共有し、二重ロードで
	/// 未保存の変更を失わないようにするため)。呼び出し側は
	/// ConfigRegistry::Get().GetManager&lt;PlayerSaveData&gt;()で取得して使う。
	/// </summary>
	inline void EnsurePlayerSaveDataLoaded()
	{
		auto& configReg = ::data::ConfigRegistry::Get();
		if (configReg.IsRegistered<PlayerSaveData>()) return;

		configReg.Register<PlayerSaveData>("Assets/Bin/Save/player_save.json");
		configReg.GetManager<PlayerSaveData>().Load();
	}
}

REFLECT_REGISTER(data::PlayerSaveData);

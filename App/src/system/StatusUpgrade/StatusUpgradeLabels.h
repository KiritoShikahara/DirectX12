#pragma once

namespace ecs::statusupgrade
{
	///<summary>
	///StatusUpgradeComponent::kOptionCount件の表示名。data::eStatUpgradeTypeの並び順と対応する。UTF-8からUTF-16への変換ユーティリティを新設せずに済むようUI表示はここで直接持つ
	///</summary>
	inline const wchar_t* GetOptionLabel(int index)
	{
		static const wchar_t* kLabels[] =
		{
			L"最大HP",
			L"攻撃力",
			L"防御力",
			L"クールダウン短縮",
			L"移動速度",
			L"ゴールド獲得量",
			L"HP自然回復",
			L"経験値獲得量",
			L"同時攻撃数",
			L"復活回数",
			L"被弾後無敵時間",
			L"パーク選択肢+1",
		};

		if (index < 0 || index >= static_cast<int>(sizeof(kLabels) / sizeof(kLabels[0])))
		{
			return L"";
		}
		return kLabels[index];
	}
}

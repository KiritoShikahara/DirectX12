#pragma once

namespace ecs::statusupgrade
{
	/// <summary>
	/// StatusUpgradeComponent::kOptionCount件の表示名。data::eStatUpgradeType(0..11)の並び順と対応する。
	/// CSV(StatUpgradeData::Name)は設計者向けの参考情報として残すが、UI表示はここで直接持つ
	/// (UTF-8→UTF-16の変換ユーティリティを新設せずに済むため。PerkDefinition::GetPerkPool()と同じ方針)。
	/// StatusUpgradeScene(カード名テキスト生成)とStatusUpgradeInputSystem(確認ダイアログ)の
	/// 両方が参照するため、どちらか一方の.cppにローカルで持たず共有ヘッダーへ切り出す。
	/// </summary>
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

#pragma once

namespace ecs
{
	/// <summary>
	/// プレイヤーのレベル・経験値を保持するコンポーネント。
	/// 敵を倒すと EnemyDeathSystem が Experience を加算し、
	/// ExperienceToNextLevel に到達するたびに GameStateComponent::PendingLevelUpCount を
	/// 加算する（1回のXP付与で複数レベル分に到達した場合はその回数分加算される）。
	/// </summary>
	struct PlayerLevelComponent
	{
		/// <summary>現在のレベル(1始まり)</summary>
		int Level = 1;

		/// <summary>現在の経験値</summary>
		float Experience = 0.0f;

		/// <summary>
		/// 次のレベルに必要な経験値。レベルアップのたびに ExperienceGrowthRate 倍で増加する
		/// （個人開発プロトタイプの暫定値。プレイ感触に応じて調整すること）。
		/// </summary>
		float ExperienceToNextLevel = 6.0f;

		/// <summary>レベルアップごとに ExperienceToNextLevel へ乗算する成長率。
		/// 1.2(20%/レベル)は複利で急激に重くなりすぎるとの指摘を受け、1.08(8%/レベル)へ緩和した
		/// （個人開発プロトタイプの暫定値。プレイ感触に応じて調整すること）。</summary>
		float ExperienceGrowthRate = 1.08f;

		/// <summary>経験値獲得量の倍率(パークで加算。基準1.0倍)。EnemyDeathSystem::AwardExperience参照</summary>
		float MulExperienceGain = 1.0f;
	};
}

#pragma once

namespace ecs
{
	///<summary>
	///プレイヤーのレベル・経験値を保持するコンポーネント。敵撃破でEnemyDeathSystemがExperienceを加算し、閾値到達のたびにGameStateComponent::PendingLevelUpCountを加算する
	///</summary>
	struct PlayerLevelComponent
	{
		///<summary>
		///現在のレベル、1始まり
		///</summary>
		int Level = 1;

		///<summary>
		///現在の経験値
		///</summary>
		float Experience = 0.0f;

		///<summary>
		///次のレベルに必要な経験値。レベルアップのたびにExperienceGrowthRate倍で増加する
		///</summary>
		float ExperienceToNextLevel = 6.0f;

		///<summary>
		///レベルアップごとにExperienceToNextLevelへ乗算する成長率
		///</summary>
		float ExperienceGrowthRate = 1.08f;

		///<summary>
		///経験値獲得量の倍率。パークで加算、基準1.0倍。EnemyDeathSystem::AwardExperience参照
		///</summary>
		float MulExperienceGain = 1.0f;
	};
}

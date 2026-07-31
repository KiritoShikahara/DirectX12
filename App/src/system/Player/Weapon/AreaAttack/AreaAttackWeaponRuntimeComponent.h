#pragma once

namespace ecs
{
	/// <summary>AreaAttack型武器専用のランタイム状態(クールダウン等)</summary>
	struct AreaAttackWeaponRuntimeComponent
	{
		/// <summary>次の手動発動(Attack2)までの残り時間(秒)。0以下で発動可能</summary>
		float CooldownTimer = 0.0f;

		/// <summary>次の自動落雷(AreaAttackAutoStrikeSystem)までの残り時間(秒)。
		/// 手動用のCooldownTimerとは完全に独立して管理する(互いに干渉しない)</summary>
		float AutoStrikeCooldownTimer = 0.0f;
	};
}

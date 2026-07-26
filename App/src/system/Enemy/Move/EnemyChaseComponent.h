#pragma once

namespace ecs
{
	/// <summary>
	/// 繝励Ξ繧､繝､繝ｼ縺ｫ蜷代°縺｣縺ｦ霑ｽ蠕薙↓縺吶ｋ謨ｵ縺ｮ繝代Λ繝｡繝ｼ繧ｿ繝ｼ
	/// </summary>
	struct EnemyChaseComponent
	{
		/// <summary>霑ｽ蟆ｾ遘ｻ蜍暮溷ｺｦ</summary>
		float MoveSpeed = 0.0f;

		/// <summary>縺薙・霍晞屬蜀・・遘ｻ蜍募・逅・ｒ縺励↑縺・/summary>
		float StopDistance = 0.0f;

	};
} 
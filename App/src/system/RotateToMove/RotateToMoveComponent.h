#pragma once
#include<Utility/Export/Export.h>

namespace ecs
{
	/// <summary>
	/// RigidBodyComponent::MoveVelocity 縺ｮ豌ｴ蟷ｳ譁ｹ蜷代↓蠢懊§縺ｦ
	/// Transform 繧貞屓霆｢縺輔○繧九◆繧√・豎守畑繧ｳ繝ｳ繝昴・繝阪Φ繝医・
	/// 繝励Ξ繧､繝､繝ｼ繝ｻ謨ｵ縲∝・縺ｫ豬∫畑蜿ｯ閭ｽ縲・
	/// </summary>
	struct ENGINE_API RotateToMoveComponent
	{
		/// <summary>1遘偵≠縺溘ｊ縺ｮ蝗櫁ｻ｢騾溷ｺｦ・亥ｺｦ・・/summary>
		float RotationSpeedDeg = 720.0f;

		/// <summary>true: 陬憺俣縺帙★蜊ｳ蠎ｧ縺ｫ逶ｮ讓呵ｧ貞ｺｦ縺ｸ蜷代￥</summary>
		bool InstantRotate = false;
	};
}
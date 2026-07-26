#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace ecs
{
	/// <summary>
	/// 迴ｾ蝨ｨ縺ｮ遘ｻ蜍墓э蝗ｳ譁ｹ蜷代ｒ陦ｨ縺呎ｱ守畑繧ｳ繝ｳ繝昴・繝阪Φ繝医・
	/// RigidBodyComponent::MoveVelocity 縺ｯ繝輔Ξ繝ｼ繝譛ｫ縺ｫ繧ｯ繝ｪ繧｢縺輔ｌ繧九◆繧√・
	/// 蝗櫁ｻ｢縺ｪ縺ｩ縲檎ｧｻ蜍墓婿蜷代阪ｒ邯咏ｶ夂噪縺ｫ蠢・ｦ√→縺吶ｋ蜃ｦ逅・・縺薙■繧峨ｒ蜿ら・縺吶ｋ縲・
	/// 繝励Ξ繧､繝､繝ｼ繝ｻ謨ｵ縲∝・縺ｫ豬∫畑蜿ｯ閭ｽ縲・
	/// </summary>
	struct ENGINE_API MoveDirectionComponent
	{
		/// <summary>豁｣隕丞喧貂医∩縺ｮ遘ｻ蜍墓婿蜷托ｼ域怙蠕後↓遘ｻ蜍輔＠縺ｦ縺・◆蜷代″繧剃ｿ晄戟・・/summary>
		DirectX::XMFLOAT3 Direction = { 0.f, 0.f, 1.f };

		/// <summary>莉翫・繝輔Ξ繝ｼ繝縺ｧ遘ｻ蜍穂ｸｭ縺九←縺・°</summary>
		bool IsMoving = false;
	};
}
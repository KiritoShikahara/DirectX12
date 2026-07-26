#pragma once

#include<Utility/Export/Export.h>
#include<algorithm>

namespace ecs
{
	/// <summary>
	/// Sprite::FillAmount 繧堤岼讓吝､縺ｸ貊代ｉ縺九↓霑ｽ蠕薙＆縺帙ｋ縺溘ａ縺ｮ繧ｳ繝ｳ繝昴・繝阪Φ繝医・
	/// 菴灘鴨繧ｲ繝ｼ繧ｸ遲峨∝､繧貞叉譎ょ渚譏縺帙★蠕舌・↓蠅玲ｸ帙＆縺帙◆縺・ｯｾ雎｡縺ｫ莉倥￠繧九・
	/// </summary>
	struct ENGINE_API FillAmountLerp
	{
		/// <summary>逶ｮ讓・FillAmount [0,1]</summary>
		float Target = 1.0f;

		/// <summary>1遘偵≠縺溘ｊ縺ｮ螟牙喧驥擾ｼ・.5f 縺ｪ繧・2遘偵〒 0竊・・・/summary>
		float Speed = 2.0f;

		/// <summary>逶ｮ讓吝､繧定ｨｭ螳壹☆繧具ｼ・0,1] 縺ｫ繧ｯ繝ｩ繝ｳ繝暦ｼ・/summary>
		void SetTarget(float t)
		{
			Target = std::clamp(t, 0.0f, 1.0f);
		}
	};
}
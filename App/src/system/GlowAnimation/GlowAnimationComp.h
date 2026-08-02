#pragma once

namespace ecs
{
    struct GlowAnimation
    {
        float BaseIntensity = 1.0f;  // 蝓ｺ貅悶→縺ｪ繧区・繧九＆
        float Amplitude = 0.5f;  // 謖ｯ蟷・ｼ医←繧後￥繧峨＞譏弱ｋ縺・證励￥縺吶ｋ縺具ｼ・
        float Frequency = 2.0f;  // 蜻ｨ豕｢謨ｰ・域・貊・・繧ｹ繝斐・繝会ｼ・
        float PhaseOffset = 0.0f;  // 菴咲嶌繧ｪ繝輔そ繝・ヨ・医ち繧､繝溘Φ繧ｰ繧偵★繧峨☆逕ｨ・・
    };
}
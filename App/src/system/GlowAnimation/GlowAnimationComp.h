#pragma once

namespace ecs
{
    struct GlowAnimation
    {
        float BaseIntensity = 1.0f;  // 基準となる明るさ
        float Amplitude = 0.5f;  // 振幅（どれくらい明るく/暗くするか）
        float Frequency = 2.0f;  // 周波数（明滅のスピード）
        float PhaseOffset = 0.0f;  // 位相オフセット（タイミングをずらす用）
    };
}
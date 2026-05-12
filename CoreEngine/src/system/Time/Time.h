#pragma once
#include<Utility/Export/Export.h>
#include<chrono>

namespace sys
{
    /// <summary>
    /// エンジン全体の時間を管理するクラス。
    /// </summary>
    class ENGINE_API Time {
    public:
        Time() = default;

        // 初期化
        void Initialize();

        // メインループの先頭で毎フレーム1回呼ぶ
        void Update();

        // タイムスケール適用後の経過時間（アニメーションや移動に使用）
        float GetDeltaTime() const { return mDeltaTime * mTimeScale; }

        // 生の経過時間（UIの演出やプロファイリングに使用）
        float GetRawDeltaTime() const { return mDeltaTime; }

        // ゲーム起動からの累積時間（シェーダーの波などの演出に使用）
        float GetTotalTime() const { return mTotalTime; }

        // 現在のタイムスケール
        float GetTimeScale() const { return mTimeScale; }

        // タイムスケールの変更（0.5fでスロー、0.0fでポーズ、2.0fで倍速）
        void SetTimeScale(float scale) { mTimeScale = (scale < 0.0f) ? 0.0f : scale; }

    private:
        // 精度の高いクロックを使用
        std::chrono::high_resolution_clock::time_point mPreviousTime;

        float mDeltaTime = 0.0f;
        float mTotalTime = 0.0f;
        float mTimeScale = 1.0f;

        // 1フレームの最大時間を0.1秒（10FPS相当）に制限するガード設定
        const float MAX_DELTA_TIME = 0.1f;
    };
}


#pragma once
#include <Utility/Export/Export.h>
#include <chrono>

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

        /// <summary> 固定更新用のデルタタイム (60FPS固定なら 約0.01666秒) </summary>
        static constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;

        /// <summary>
        /// Update()1回あたりに許容する固定ステップの最大実行回数。
        /// フレームレートが大きく落ち込むと、蓄積時間(MAX_DELTA_TIME=0.1秒分まで)を
        /// 追いつかせようとして物理ステップを連続実行することになるが、物理ステップ自体が
        /// 重い場合、これがさらにフレームを重くし、フレームレートの悪化を加速させてしまう
        /// (いわゆるspiral of death)。この上限に達したら、追いつききれなかった時間は
        /// (次フレームへ持ち越さず)そのフレームでは諦める＝物理がやや遅れて見えることを
        /// 許容し、フレームレートの底割れを防ぐことを優先する。
        /// </summary>
        static constexpr int MAX_FIXED_STEPS_PER_UPDATE = 3;

        /// <summary>
        /// 固定更新のステップを実行すべきか判定し、実行する場合はTrueを返し時間を消費する。
        /// while(time.AccumulateFixedStep()) { ... } のように使用する。
        /// MAX_FIXED_STEPS_PER_UPDATEに達した場合はfalseを返す(Update()内でリセットされる)。
        /// </summary>
        bool AccumulateFixedStep();

        /// <summary> 固定ステップ用のタイムスケール適用済みデルタタイム </summary>
        float GetFixedDeltaTime() const { return FIXED_DELTA_TIME * mTimeScale; }

    private:
        // 精度の高いクロックを使用
        std::chrono::high_resolution_clock::time_point mPreviousTime;

        float mDeltaTime = 0.0f;
        float mTotalTime = 0.0f;
        float mTimeScale = 1.0f;

        // 1フレームの最大時間を0.1秒（10FPS相当）に制限するガード設定
        const float MAX_DELTA_TIME = 0.1f;

        // ⚡ 物理用の時間蓄積バッファ
        float mPhysicsAccumulator = 0.0f;

        // 今回のUpdate()で既に実行した固定ステップの回数(MAX_FIXED_STEPS_PER_UPDATEとの比較用、
        // Update()の先頭で0にリセットする)
        int mFixedStepsThisUpdate = 0;
    };
}
#include"pch.h"
#include "Time.h"

namespace sys
{
    void Time::Initialize() {
        mPreviousTime = std::chrono::high_resolution_clock::now();
        mDeltaTime = 0.0f;
        mTotalTime = 0.0f;
        mTimeScale = 1.0f;
    }

    void Time::Update() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = currentTime - mPreviousTime;
        mPreviousTime = currentTime;

        mDeltaTime = deltaTime.count();

        if (mDeltaTime > MAX_DELTA_TIME)
        {
            mDeltaTime = MAX_DELTA_TIME;
        }

        mTotalTime += mDeltaTime * mTimeScale;

        mPhysicsAccumulator += mDeltaTime * mTimeScale;

        if (mPhysicsAccumulator > MAX_DELTA_TIME)
        {
            mPhysicsAccumulator = MAX_DELTA_TIME;
        }

        // 今フレーム分の固定ステップ実行回数をリセットする
        mFixedStepsThisUpdate = 0;
    }

    bool Time::AccumulateFixedStep()
    {
        // 1回のUpdate()あたりの実行回数が上限に達していたら、蓄積時間が残っていても
        // 今フレームはこれ以上実行しない。残りの蓄積時間は次フレームへ
        if (mFixedStepsThisUpdate >= MAX_FIXED_STEPS_PER_UPDATE)
        {
            return false;
        }

        // 蓄積された時間が固定ステップの歩幅を超えていれば、1回分のステップを許可
        if (mPhysicsAccumulator >= FIXED_DELTA_TIME)
        {
            mPhysicsAccumulator -= FIXED_DELTA_TIME;
            ++mFixedStepsThisUpdate;
            return true;
        }
        return false;
    }
}
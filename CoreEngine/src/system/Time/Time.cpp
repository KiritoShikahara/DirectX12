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

        // 前フレームからの経過時間を計算
        std::chrono::duration<float> elapsed = currentTime - mPreviousTime;
        mPreviousTime = currentTime;

        float rawFrameTime = elapsed.count();

        // 処理落ちやデバッグ停止による異常な長時間（deltaTime）をカットする
        if (rawFrameTime > MAX_DELTA_TIME) {
            mDeltaTime = MAX_DELTA_TIME;
        }
        else {
            mDeltaTime = rawFrameTime;
        }

        mTotalTime += mDeltaTime;
    }
}
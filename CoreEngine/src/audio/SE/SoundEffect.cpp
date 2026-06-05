#include "pch.h"
#include "SoundEffect.h"
#include"../Resource/AudioResource.h"


namespace audio
{
	SoundEffect::SoundEffect(AudioResource* resource)
		: mResource(resource)
	{
	}

    // 移動コンストラクタのカスタム実装
    SoundEffect::SoundEffect(SoundEffect&& other) noexcept
    {
        mResource = other.mResource;
        mCurrentFrame = other.mCurrentFrame;
        mIsPersistent = other.mIsPersistent;

        // atomic は load して初期化（other から値を読み出す）
        mVolume.store(other.mVolume.load());
        mLoop.store(other.mLoop.load());
        mPlaying.store(other.mPlaying.load());

        // 移動元のポインタなどはクリアしておく（二重解放などの防止）
        other.mResource = nullptr;
        other.mCurrentFrame = 0;
        other.mPlaying.store(false);
    }

    // 移動代入演算子のカスタム実装
    SoundEffect& SoundEffect::operator=(SoundEffect&& other) noexcept
    {
        if (this != &other)
        {
            mResource = other.mResource;
            mCurrentFrame = other.mCurrentFrame;
            mIsPersistent = other.mIsPersistent;

            mVolume.store(other.mVolume.load());
            mLoop.store(other.mLoop.load());
            mPlaying.store(other.mPlaying.load());

            other.mResource = nullptr;
            other.mCurrentFrame = 0;
            other.mPlaying.store(false);
        }
        return *this;
    }

	void SoundEffect::ApplyAndMix(int16_t* output, size_t framesRequested, uint16_t outputChannels, float masterVolume, float seVolume)
	{
		if (!mPlaying.load() || mResource == nullptr || mResource->Channels == 0) return;

		const size_t   totalFrames = mResource->FrameCount();
		const uint16_t srcChannels = mResource->Channels;
		const size_t   framesToCopy = std::min(framesRequested, totalFrames - mCurrentFrame);

        // ボリュームバスを適用
        const float finalVolume = mVolume.load() * seVolume * masterVolume;

        for (size_t frame = 0; frame < framesToCopy; ++frame)
        {
            for (uint16_t ch = 0; ch < outputChannels; ++ch)
            {
                int32_t sample = 0;

                // ダウンミックス処理: 2ch(ステレオ)ソースを 1ch(モノラル)デバイスに出力する場合
                if (srcChannels == 2 && outputChannels == 1)
                {
                    const int32_t s0 = mResource->PcmData[(mCurrentFrame + frame) * 2 + 0];
                    const int32_t s1 = mResource->PcmData[(mCurrentFrame + frame) * 2 + 1];
                    sample = static_cast<int32_t>((s0 + s1) / 2.0f * finalVolume);
                }
                else
                {
                    // 通常のチャンネルマッピング
                    const uint16_t srcCh = std::min(ch, static_cast<uint16_t>(srcChannels - 1));
                    sample = static_cast<int32_t>(mResource->PcmData[(mCurrentFrame + frame) * srcChannels + srcCh] * finalVolume);
                }

                // 飽和加算（クランプ）
                const int32_t mixed = static_cast<int32_t>(output[frame * outputChannels + ch]) + sample;
                output[frame * outputChannels + ch] = static_cast<int16_t>(
                    std::clamp(mixed, static_cast<int32_t>(INT16_MIN), static_cast<int32_t>(INT16_MAX)));
            }
        }

        mCurrentFrame += framesToCopy;

        if (mCurrentFrame >= totalFrames)
        {
            if (mLoop.load())
            {
                mCurrentFrame = 0;
            }
            else
            {
                mPlaying.store(false);
            }
        }
	}


}
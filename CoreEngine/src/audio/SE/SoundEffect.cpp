#include "pch.h"
#include "SoundEffect.h"
#include"../Resource/AudioResource.h"


namespace audio
{
	SoundEffect::SoundEffect(AudioResource* resource)
		: mResource(resource)
	{
	}

    // �ړ��R���X�g���N�^�̃J�X�^������
    SoundEffect::SoundEffect(SoundEffect&& other) noexcept
    {
        mResource = other.mResource;
        mCurrentFrame = other.mCurrentFrame;
        mIsPersistent = other.mIsPersistent;

        mVolume.store(other.mVolume.load());
        mLoop.store(other.mLoop.load());
        mPlaying.store(other.mPlaying.load());

        // �ړ����̃|�C���^�Ȃǂ̓N���A���Ă���
        other.mResource = nullptr;
        other.mCurrentFrame = 0;
        other.mPlaying.store(false);
    }

    // �ړ�������Z�q�̃J�X�^������
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

	void SoundEffect::ApplyAndMix(float* output, size_t framesRequested, uint16_t outputChannels, float masterVolume, float seVolume)
	{
		if (!mPlaying.load() || mResource == nullptr || mResource->Channels == 0) return;

		const size_t   totalFrames = mResource->FrameCount();
		const uint16_t srcChannels = mResource->Channels;
		const size_t   framesToCopy = std::min(framesRequested, totalFrames - mCurrentFrame);

        // �{�����[���o�X��K�p
        const float finalVolume = mVolume.load() * seVolume * masterVolume;

        for (size_t frame = 0; frame < framesToCopy; ++frame)
        {
            for (uint16_t ch = 0; ch < outputChannels; ++ch)
            {
                int32_t sample = 0;

                // �_�E���~�b�N�X����: 2ch(�X�e���I)�\�[�X�� 1ch(���m����)�f�o�C�X�ɏo�͂���ꍇ
                if (srcChannels == 2 && outputChannels == 1)
                {
                    const int32_t s0 = mResource->PcmData[(mCurrentFrame + frame) * 2 + 0];
                    const int32_t s1 = mResource->PcmData[(mCurrentFrame + frame) * 2 + 1];
                    sample = static_cast<int32_t>((s0 + s1) / 2.0f * finalVolume);
                }
                else
                {
                    // �ʏ�̃`�����l���}�b�s���O
                    const uint16_t srcCh = std::min(ch, static_cast<uint16_t>(srcChannels - 1));
                    sample = static_cast<int32_t>(mResource->PcmData[(mCurrentFrame + frame) * srcChannels + srcCh] * finalVolume);
                }

                // クリップせず加算するだけ(最終段でAudioManagerがリミッターをかけてint16化する)
                output[frame * outputChannels + ch] += static_cast<float>(sample);
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
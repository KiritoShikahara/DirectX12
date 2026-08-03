#include "pch.h"
#include "BGMStream.h"

namespace audio
{

    BGMStream::BGMStream(std::string filePath)
        : mFilePath(std::move(filePath))
    {
    }

    BGMStream::~BGMStream()
	{
		Stop();
	}

	bool BGMStream::Open()
	{
        std::lock_guard lock(mFileMtx);

        mFileStream.open(mFilePath, std::ios::binary);
        if (!mFileStream)
        {
            return false;
        }

        mFileStream.read(reinterpret_cast<char*>(&mHeader), sizeof(AudioHeader));
        if (!mFileStream)
        {
            mFileStream.close();
            return false;
        }

        // �o���f�[�V����
        if (mHeader.Channels == 0 || mHeader.FrameCount == 0 || mHeader.SampleRate == 0)
        {
            mFileStream.close();
            return false;
        }

        // �����O�o�b�t�@���m�ہi�t�@�C���̃`�����l�����Ŋm��j
        mRingCapFrames = kRingBufferFrames;
        mRingBuffer.assign(mRingCapFrames * mHeader.Channels, 0);

        mWritePos.store(0, std::memory_order_relaxed);
        mReadPos.store(0, std::memory_order_relaxed);

        return true;
    }

    void BGMStream::Play()
    {
        if (!mFileStream.is_open()) return;

        mPlaying.store(true, std::memory_order_relaxed);

        // ���łɃX���b�h�������Ă���΍ċN�����Ȃ��iPause �� Play �̕��A�j
        if (mLoaderThread.joinable()) return;

        // ���[�_�[�X���b�h���N��
        mLoaderThread = std::jthread([this](std::stop_token st)
            {
                LoaderThread(std::move(st));
            });
    }

    void BGMStream::Stop()
    {
        // �Đ��t���O�𗎂Ƃ�
        mPlaying.store(false, std::memory_order_relaxed);

        // jthread �ɒ�~��v�� �� LoaderThread �� wait ���N����
        mLoaderThread.request_stop();
        mLoaderCV.notify_all();

        // jthread �̃f�X�g���N�^�Ŏ��� join
        if (mLoaderThread.joinable())
        {
            mLoaderThread.join();
        }

        // �t�@�C���ʒu��擪��PCM�f�[�^�փ��Z�b�g
        {
            std::lock_guard lock(mFileMtx);
            if (mFileStream.is_open())
            {
                mFileStream.clear();
                mFileStream.seekg(kPcmOffset, std::ios::beg);
            }
        }

        // �����O�o�b�t�@�����Z�b�g
        mWritePos.store(0, std::memory_order_relaxed);
        mReadPos.store(0, std::memory_order_relaxed);
    }

    void BGMStream::ApplyAndMix(float* output, size_t framesRequested, uint16_t outputChannels, float masterVolume, float bgmVolume)
    {
        if (!mPlaying.load(std::memory_order_relaxed)) return;

        const uint16_t srcChannels = mHeader.Channels;
        const float    finalVolume = mVolume.load(std::memory_order_relaxed) * bgmVolume * masterVolume;

        size_t r = mReadPos.load(std::memory_order_relaxed);

        size_t framesMixed = 0;

        while (framesMixed < framesRequested)
        {
            const size_t readable = (mWritePos.load(std::memory_order_acquire) - r + mRingCapFrames) % mRingCapFrames;

            if (readable == 0)
            {
                // �o�b�t�@�A���_�[����: �c��𖳉��Ŗ��߂ă��[�_�[���N����
                // output �͌Ăяo������ 0 �N���A�ς݂Ȃ̂Œǉ������s�v
                mLoaderCV.notify_one(); // ���[�_�[�𑁂߂ɋN�����q���g
                break;
            }

            const size_t framesToMix = std::min(framesRequested - framesMixed, readable);

            for (size_t f = 0; f < framesToMix; ++f)
            {
                const size_t ringFrame = r % mRingCapFrames;
                const size_t srcBase = ringFrame * srcChannels;
                const size_t outBase = (framesMixed + f) * outputChannels;

                for (uint16_t ch = 0; ch < outputChannels; ++ch)
                {
                    int32_t sample = 0;

                    if (srcChannels == 1)
                    {
                        // ���m�����\�[�X �� �S�o�̓`�����l���֕���
                        sample = static_cast<int32_t>(mRingBuffer[srcBase] * finalVolume);
                    }
                    else if (srcChannels >= 2 && outputChannels == 1)
                    {
                        // �X�e���I �� ���m���� �_�E���~�b�N�X�iL+R �̕��ρj
                        const int32_t l = mRingBuffer[srcBase + 0];
                        const int32_t r2 = mRingBuffer[srcBase + 1];
                        sample = static_cast<int32_t>((l + r2) * 0.5f * finalVolume);
                    }
                    else
                    {
                        // �ʏ�: �\�[�X�̃`�����l�������o�͂�菭�Ȃ��ꍇ�͍ŏIch�ŕ⊮
                        const uint16_t srcCh = std::min(ch, static_cast<uint16_t>(srcChannels - 1));
                        sample = static_cast<int32_t>(mRingBuffer[srcBase + srcCh] * finalVolume);
                    }

                    // クリップせず加算するだけ(最終段でAudioManagerがリミッターをかけてint16化する)
                    output[outBase + ch] += static_cast<float>(sample);
                }

                ++r;
            }

            framesMixed += framesToMix;
        }

        // �R�[���o�b�N�X���b�h��������ʒu�����[�_�[�֒ʒm�irelease�j
        mReadPos.store(r, std::memory_order_release);

        // �o�b�t�@�ɋ󂫂��ł����̂Ń��[�_�[���N����
        mLoaderCV.notify_one();
    }

    void BGMStream::LoaderThread(std::stop_token stopToken)
    {
        // �t�@�C���ǂݍ��ݗp�̈ꎞ�o�b�t�@
        std::vector<int16_t> readBuf;

        while (!stopToken.stop_requested())
        {
            const size_t writable = WritableFrames();

            if (writable == 0)
            {
                // �o�b�t�@�����t �� �����󂭂܂őҋ@
                std::unique_lock lk(mLoaderCVMtx);
                mLoaderCV.wait_for(lk, std::chrono::milliseconds(2),
                    [&] { return stopToken.stop_requested() || WritableFrames() > 0; });
                continue;
            }

            // ��x�ɏ����t���[�����i�o�b�t�@�̔�����ڈ��Ɂj
            const size_t framesToLoad = std::min(writable, mRingCapFrames / 2);
            const size_t elemsToRead = framesToLoad * mHeader.Channels;

            if (readBuf.size() < elemsToRead)
            {
                readBuf.resize(elemsToRead);
            }

            // �t�@�C������ǂݍ���
            size_t framesRead = 0;
            {
                std::lock_guard lock(mFileMtx);

                mFileStream.read(reinterpret_cast<char*>(readBuf.data()),
                    static_cast<std::streamsize>(elemsToRead * sizeof(int16_t)));

                const std::streamsize bytesRead = mFileStream.gcount();
                framesRead = static_cast<size_t>(bytesRead) / (mHeader.Channels * sizeof(int16_t));

                // �t�@�C���������B
                if (framesRead < framesToLoad)
                {
                    if (mLoop.load(std::memory_order_relaxed))
                    {
                        // ���[�v: PCM �擪�փV�[�N
                        mFileStream.clear();
                        mFileStream.seekg(kPcmOffset, std::ios::beg);
                        // �ǂݎc���͍��񕪂��������Ď����[�v�Ŏc���ǂ�
                    }
                    else
                    {
                        // �񃋁[�v: �ǂ߂������������ďI��
                        // framesRead == 0 �Ȃ牽�����Ȃ�
                    }
                }
            }

            if (framesRead == 0)
            {
                if (!mLoop.load(std::memory_order_relaxed))
                {
                    // �������B���񃋁[�v �� �Đ��I��
                    mPlaying.store(false, std::memory_order_relaxed);
                    break;
                }
                continue;
            }

            // �����O�o�b�t�@�֏�������
            size_t w = mWritePos.load(std::memory_order_relaxed);

            for (size_t f = 0; f < framesRead; ++f)
            {
                const size_t ringFrame = w % mRingCapFrames;
                const size_t srcBase = f * mHeader.Channels;
                const size_t dstBase = ringFrame * mHeader.Channels;

                for (uint16_t ch = 0; ch < mHeader.Channels; ++ch)
                {
                    mRingBuffer[dstBase + ch] = readBuf[srcBase + ch];
                }
                ++w;
            }

            // �R�[���o�b�N�X���b�h�֏��������Ƃ�ʒm
            mWritePos.store(w, std::memory_order_release);
        }
    }

    size_t BGMStream::WritableFrames() const noexcept
    {
        // ���[�_�[�X���b�h�݂̂��Ă�
        // write �� read �̃L���b�v�� = �󂫗e��
        // -1 ���Ė��t�Ƌ����ʂ���i�t���o�b�t�@�͗e��-1�t���[���܂Łj
        const size_t w = mWritePos.load(std::memory_order_relaxed);
        const size_t r = mReadPos.load(std::memory_order_acquire); // �R�[���o�b�N���̏������݂�����
        const size_t used = (w - r + mRingCapFrames) % mRingCapFrames;
        return (mRingCapFrames - 1) - used;
    }
    size_t BGMStream::ReadableFrames() const noexcept
    {
        // �R�[���o�b�N�X���b�h�݂̂��Ă�
        const size_t w = mWritePos.load(std::memory_order_acquire); // ���[�_�[���̏������݂�����
        const size_t r = mReadPos.load(std::memory_order_relaxed);
        return (w - r + mRingCapFrames) % mRingCapFrames;
    }
}
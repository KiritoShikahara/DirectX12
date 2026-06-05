#include "pch.h"
#include "BGMStream.h"

namespace audio
{
	BGMStream::BGMStream(const std::string& filePath)
		: mFilePath(filePath)
	{
	}

	BGMStream::~BGMStream()
	{
		Stop();
	}

	bool BGMStream::Open()
	{
        mFileStream.open(mFilePath, std::ios::binary);
        if (!mFileStream) return false;

        mFileStream.read(reinterpret_cast<char*>(&mHeader), sizeof(AudioHeader));
        if (!mFileStream)
        {
            mFileStream.close();
            return false;
        }

        if (mHeader.Channels == 0 || mHeader.FrameCount == 0)
        {
            mFileStream.close();
            return false;
        }

        mCurrentFrame = 0;
        return true;
    }

    void BGMStream::Stop()
    {
        mPlaying.store(false);
        std::lock_guard lock(mStreamMtx);
        if (mFileStream.is_open())
        {
            mFileStream.clear();
            mFileStream.seekg(sizeof(AudioHeader), std::ios::beg);
        }
        mCurrentFrame = 0;
    }

    void BGMStream::ApplyAndMix(int16_t* output, size_t framesRequested, uint16_t outputChannels, float masterVolume, float bgmVolume)
    {
        if (!mPlaying.load() || !mFileStream.is_open() || mHeader.Channels == 0) return;

        std::lock_guard lock(mStreamMtx); // スレッドセーフ確保

        const uint16_t srcChannels = mHeader.Channels;
        const float finalVolume = mVolume.load() * bgmVolume * masterVolume;

        size_t framesMixed = 0;

        while (framesMixed < framesRequested)
        {
            const size_t framesAvailable = static_cast<size_t>(mHeader.FrameCount - mCurrentFrame);

            // ファイル末尾に達した場合
            if (framesAvailable == 0)
            {
                if (mLoop.load())
                {
                    mFileStream.clear();
                    mFileStream.seekg(sizeof(AudioHeader), std::ios::beg); // データ開始位置に戻る
                    mCurrentFrame = 0;
                    continue;
                }
                else
                {
                    mPlaying.store(false);
                    break;
                }
            }

            const size_t framesToCopy = std::min(framesRequested - framesMixed, framesAvailable);
            const size_t elementsToRead = framesToCopy * srcChannels;

            // 必要に応じて一時ロードバッファを拡張
            if (mReadBuffer.size() < elementsToRead)
            {
                mReadBuffer.resize(elementsToRead);
            }

            // ファイルから部分シーク＆ロード
            mFileStream.read(reinterpret_cast<char*>(mReadBuffer.data()),
                static_cast<std::streamsize>(elementsToRead * sizeof(int16_t)));

            std::streamsize bytesRead = mFileStream.gcount();
            size_t actualFramesRead = static_cast<size_t>(bytesRead / (srcChannels * sizeof(int16_t)));

            if (actualFramesRead == 0)
            {
                mPlaying.store(false); // 読み込みエラー時は強制停止
                break;
            }

            // 読み込んだデータをミキシング
            for (size_t frame = 0; frame < actualFramesRead; ++frame)
            {
                const size_t outFrameIdx = framesMixed + frame;

                for (uint16_t ch = 0; ch < outputChannels; ++ch)
                {
                    int32_t sample = 0;

                    // ダウンミックス
                    if (srcChannels == 2 && outputChannels == 1)
                    {
                        const int32_t s0 = mReadBuffer[frame * 2 + 0];
                        const int32_t s1 = mReadBuffer[frame * 2 + 1];
                        sample = static_cast<int32_t>((s0 + s1) / 2.0f * finalVolume);
                    }
                    else
                    {
                        const uint16_t srcCh = std::min(ch, static_cast<uint16_t>(srcChannels - 1));
                        sample = static_cast<int32_t>(mReadBuffer[frame * srcChannels + srcCh] * finalVolume);
                    }

                    const int32_t mixed = static_cast<int32_t>(output[outFrameIdx * outputChannels + ch]) + sample;
                    output[outFrameIdx * outputChannels + ch] = static_cast<int16_t>(
                        std::clamp(mixed, static_cast<int32_t>(INT16_MIN), static_cast<int32_t>(INT16_MAX)));
                }
            }

            mCurrentFrame += actualFramesRead;
            framesMixed += actualFramesRead;
        }
    }
}
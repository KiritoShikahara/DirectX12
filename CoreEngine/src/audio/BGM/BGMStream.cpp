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

        // バリデーション
        if (mHeader.Channels == 0 || mHeader.FrameCount == 0 || mHeader.SampleRate == 0)
        {
            mFileStream.close();
            return false;
        }

        // リングバッファを確保（ファイルのチャンネル数で確定）
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

        // すでにスレッドが動いていれば再起動しない（Pause → Play の復帰）
        if (mLoaderThread.joinable()) return;

        // ローダースレッドを起動
        mLoaderThread = std::jthread([this](std::stop_token st)
            {
                LoaderThread(std::move(st));
            });
    }

    void BGMStream::Stop()
    {
        // 再生フラグを落とす
        mPlaying.store(false, std::memory_order_relaxed);

        // jthread に停止を要求 → LoaderThread の wait を起こす
        mLoaderThread.request_stop();
        mLoaderCV.notify_all();

        // jthread のデストラクタで自動 join
        if (mLoaderThread.joinable())
        {
            mLoaderThread.join();
        }

        // ファイル位置を先頭のPCMデータへリセット
        {
            std::lock_guard lock(mFileMtx);
            if (mFileStream.is_open())
            {
                mFileStream.clear();
                mFileStream.seekg(kPcmOffset, std::ios::beg);
            }
        }

        // リングバッファをリセット
        mWritePos.store(0, std::memory_order_relaxed);
        mReadPos.store(0, std::memory_order_relaxed);
    }

    void BGMStream::ApplyAndMix(int16_t* output, size_t framesRequested, uint16_t outputChannels, float masterVolume, float bgmVolume)
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
                // バッファアンダーラン: 残りを無音で埋めてローダーを起こす
                // output は呼び出し元で 0 クリア済みなので追加処理不要
                mLoaderCV.notify_one(); // ローダーを早めに起こすヒント
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
                        // モノラルソース → 全出力チャンネルへ複製
                        sample = static_cast<int32_t>(mRingBuffer[srcBase] * finalVolume);
                    }
                    else if (srcChannels >= 2 && outputChannels == 1)
                    {
                        // ステレオ → モノラル ダウンミックス（L+R の平均）
                        const int32_t l = mRingBuffer[srcBase + 0];
                        const int32_t r2 = mRingBuffer[srcBase + 1];
                        sample = static_cast<int32_t>((l + r2) * 0.5f * finalVolume);
                    }
                    else
                    {
                        // 通常: ソースのチャンネル数が出力より少ない場合は最終chで補完
                        const uint16_t srcCh = std::min(ch, static_cast<uint16_t>(srcChannels - 1));
                        sample = static_cast<int32_t>(mRingBuffer[srcBase + srcCh] * finalVolume);
                    }

                    const int32_t mixed = static_cast<int32_t>(output[outBase + ch]) + sample;
                    output[outBase + ch] = static_cast<int16_t>(
                        std::clamp(mixed,
                            static_cast<int32_t>(INT16_MIN),
                            static_cast<int32_t>(INT16_MAX)));
                }

                ++r;
            }

            framesMixed += framesToMix;
        }

        // コールバックスレッドが消費した位置をローダーへ通知（release）
        mReadPos.store(r, std::memory_order_release);

        // バッファに空きができたのでローダーを起こす
        mLoaderCV.notify_one();
    }

    void BGMStream::LoaderThread(std::stop_token stopToken)
    {
        // ファイル読み込み用の一時バッファ
        std::vector<int16_t> readBuf;

        while (!stopToken.stop_requested())
        {
            const size_t writable = WritableFrames();

            if (writable == 0)
            {
                // バッファが満杯 → 少し空くまで待機
                std::unique_lock lk(mLoaderCVMtx);
                mLoaderCV.wait_for(lk, std::chrono::milliseconds(2),
                    [&] { return stopToken.stop_requested() || WritableFrames() > 0; });
                continue;
            }

            // 一度に書くフレーム数（バッファの半分を目安に）
            const size_t framesToLoad = std::min(writable, mRingCapFrames / 2);
            const size_t elemsToRead = framesToLoad * mHeader.Channels;

            if (readBuf.size() < elemsToRead)
            {
                readBuf.resize(elemsToRead);
            }

            // ファイルから読み込む
            size_t framesRead = 0;
            {
                std::lock_guard lock(mFileMtx);

                mFileStream.read(reinterpret_cast<char*>(readBuf.data()),
                    static_cast<std::streamsize>(elemsToRead * sizeof(int16_t)));

                const std::streamsize bytesRead = mFileStream.gcount();
                framesRead = static_cast<size_t>(bytesRead) / (mHeader.Channels * sizeof(int16_t));

                // ファイル末尾到達
                if (framesRead < framesToLoad)
                {
                    if (mLoop.load(std::memory_order_relaxed))
                    {
                        // ループ: PCM 先頭へシーク
                        mFileStream.clear();
                        mFileStream.seekg(kPcmOffset, std::ios::beg);
                        // 読み残しは今回分だけ書いて次ループで残りを読む
                    }
                    else
                    {
                        // 非ループ: 読めた分だけ書いて終了
                        // framesRead == 0 なら何もしない
                    }
                }
            }

            if (framesRead == 0)
            {
                if (!mLoop.load(std::memory_order_relaxed))
                {
                    // 末尾到達かつ非ループ → 再生終了
                    mPlaying.store(false, std::memory_order_relaxed);
                    break;
                }
                continue;
            }

            // リングバッファへ書き込む
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

            // コールバックスレッドへ書いたことを通知
            mWritePos.store(w, std::memory_order_release);
        }
    }

    size_t BGMStream::WritableFrames() const noexcept
    {
        // ローダースレッドのみが呼ぶ
        // write と read のキャップ差 = 空き容量
        // -1 して満杯と空を区別する（フルバッファは容量-1フレームまで）
        const size_t w = mWritePos.load(std::memory_order_relaxed);
        const size_t r = mReadPos.load(std::memory_order_acquire); // コールバック側の書き込みを見る
        const size_t used = (w - r + mRingCapFrames) % mRingCapFrames;
        return (mRingCapFrames - 1) - used;
    }
    size_t BGMStream::ReadableFrames() const noexcept
    {
        // コールバックスレッドのみが呼ぶ
        const size_t w = mWritePos.load(std::memory_order_acquire); // ローダー側の書き込みを見る
        const size_t r = mReadPos.load(std::memory_order_relaxed);
        return (w - r + mRingCapFrames) % mRingCapFrames;
    }
}
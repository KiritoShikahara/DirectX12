#pragma once
#include"../Data/AudioHeader.h"

#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdint>

namespace audio
{
    /// <summary>
    /// BGM ストリーミング再生クラス
    /// </summary>
    class BGMStream
    {
    public:
        /// <summary>リングバッファのフレーム数（2の累乗推奨）</summary>
        static constexpr size_t kRingBufferFrames = 8192;

        explicit BGMStream(std::string filePath);
        ~BGMStream();

        // コピー・ムーブ禁止（jthread と atomic を持つため）
        BGMStream(const BGMStream&) = delete;
        BGMStream& operator=(const BGMStream&) = delete;
        BGMStream(BGMStream&&) = delete;
        BGMStream& operator=(BGMStream&&) = delete;

        /// <summary>
        /// ファイルを開いてヘッダーを読み込む。
        /// Play() の前に必ず呼ぶこと。
        /// </summary>
        /// <returns>成功時 true</returns>
        bool Open();

        /// <summary>再生開始（ローダースレッドを起動する）</summary>
        void Play();

        /// <summary>再生位置を先頭に戻して停止。ローダースレッドを安全に終了させる</summary>
        void Stop();

        /// <summary>再生位置を維持したまま一時停止</summary>
        void Pause() noexcept { mPlaying.store(false, std::memory_order_relaxed); }

        // 再生パラメーター
        [[nodiscard]] bool  IsPlaying() const noexcept { return mPlaying.load(std::memory_order_relaxed); }
        [[nodiscard]] float Volume()    const noexcept { return mVolume.load(std::memory_order_relaxed); }
        [[nodiscard]] bool  IsLoop()    const noexcept { return mLoop.load(std::memory_order_relaxed); }

        void SetVolume(float v) noexcept { mVolume.store(v, std::memory_order_relaxed); }
        void SetLoop(bool  l)   noexcept { mLoop.store(l, std::memory_order_relaxed); }

        /// <summary>
        /// リングバッファから PCM を読み出して output へ加算ミックスする。
        /// </summary>
        /// <param name="output">出力バッファ（インターリーブ形式）</param>
        /// <param name="framesRequested">要求フレーム数</param>
        /// <param name="outputChannels">出力チャンネル数</param>
        /// <param name="masterVolume">マスター音量</param>
        /// <param name="bgmVolume">BGMカテゴリ音量</param>
        void ApplyAndMix(int16_t* output,
            size_t   framesRequested,
            uint16_t outputChannels,
            float    masterVolume,
            float    bgmVolume);

        // メタ情報
        [[nodiscard]] uint32_t SampleRate() const noexcept { return mHeader.SampleRate; }
        [[nodiscard]] uint16_t Channels()   const noexcept { return mHeader.Channels; }

    private:
        // ローダースレッド

        /// <summary>jthread のエントリポイント。stop_token で安全に終了する</summary>
        void LoaderThread(std::stop_token stopToken);

        /// <summary>リングバッファへ書き込み可能なフレーム数を返す（ローダー側から呼ぶ）</summary>
        [[nodiscard]] size_t WritableFrames() const noexcept;

        /// <summary>リングバッファから読み出し可能なフレーム数を返す（コールバック側から呼ぶ）</summary>
        [[nodiscard]] size_t ReadableFrames() const noexcept;

        // メンバ変数
        const std::string mFilePath;
        std::ifstream     mFileStream;
        AudioHeader       mHeader = {};

        // リングバッファ
        // サイズ = kRingBufferFrames * Channels（Open後に確定）
        std::vector<int16_t>  mRingBuffer;
        size_t                mRingCapFrames = 0; // リングバッファのフレーム容量

        // write: ローダースレッドのみ書く  read: コールバックのみ書く
        std::atomic<size_t>   mWritePos{ 0 }; // 単位: フレーム
        std::atomic<size_t>   mReadPos{ 0 }; // 単位: フレーム

        // ローダースレッド制御
        std::jthread            mLoaderThread;
        std::condition_variable mLoaderCV;
        std::mutex              mLoaderCVMtx; // CV 専用（ファイルや PCM へのアクセスは別管理）

        // ファイルシーク保護
        // ローダースレッドと Stop() が同時にシークしないよう守る軽量 mutex
        std::mutex mFileMtx;

        // 状態フラグ
        std::atomic<bool>  mPlaying{ false };
        std::atomic<bool>  mLoop{ true };
        std::atomic<float> mVolume{ 1.0f };

        // ファイル先頭からPCMデータが始まるオフセット（= sizeof(AudioHeader)）
        static constexpr std::streamoff kPcmOffset = static_cast<std::streamoff>(sizeof(AudioHeader));
    };

} // namespace audio
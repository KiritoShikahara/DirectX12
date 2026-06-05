#pragma once

#include<string>
#include<vector>
#include<fstream>
#include<atomic>
#include<mutex>
#include"../Data/AudioHeader.h"

namespace audio
{
	class BGMStream
	{
	public:
		explicit BGMStream(const std::string& filePath);
		~BGMStream();

		/// <summary>ファイルをオープンし、ヘッダー情報を読み込む</summary>
		bool Open();

		void Play()  noexcept { mPlaying.store(true); }
		void Stop();
		void Pause() noexcept { mPlaying.store(false); }

		[[nodiscard]] bool  IsPlaying() const noexcept { return mPlaying.load(); }
		[[nodiscard]] float Volume()    const noexcept { return mVolume.load(); }
		[[nodiscard]] bool  IsLoop()    const noexcept { return mLoop.load(); }

		void SetVolume(float volume) noexcept { mVolume.store(volume); }
		void SetLoop(bool loop)      noexcept { mLoop.store(loop); }

		/// <summary>
		/// ミキシング。ディスクから必要なフレームを都度ロードしながらミックスします。
		/// </summary>
		void ApplyAndMix(int16_t* output, size_t framesRequested, uint16_t outputChannels, float masterVolume, float bgmVolume);

	private:
		std::string   mFilePath;
		std::ifstream mFileStream;
		AudioHeader   mHeader = {};
		uint64_t      mCurrentFrame = 0;

		std::atomic<float> mVolume{ 1.0f };
		std::atomic<bool>  mLoop{ true };
		std::atomic<bool>  mPlaying{ false };

		std::vector<int16_t> mReadBuffer; // ストリーミング用一時バッファ
		std::mutex           mStreamMtx;  // ファイルポインタ操作の競合防止用


	};
}



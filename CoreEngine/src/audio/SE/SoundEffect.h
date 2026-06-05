#pragma once

#include <cstdint>
#include <atomic>

namespace audio
{
	struct AudioResource;

	class SoundEffect
	{
	public:
		explicit SoundEffect(AudioResource* resource);
		~SoundEffect() = default;

		// 再生・停止・一時停止
		void Play()  noexcept { mPlaying.store(true); }
		void Stop()  noexcept { mPlaying.store(false); mCurrentFrame = 0; }
		void Pause() noexcept { mPlaying.store(false); }

		// アクセサ
		[[nodiscard]] bool  IsPlaying()    const noexcept { return mPlaying.load(); }
		[[nodiscard]] bool  IsPersistent() const noexcept { return mIsPersistent; }
		[[nodiscard]] float Volume()       const noexcept { return mVolume.load(); }
		[[nodiscard]] bool  IsLoop()       const noexcept { return mLoop.load(); }

		void SetVolume(float volume)        noexcept { mVolume.store(volume); }
		void SetLoop(bool loop)             noexcept { mLoop.store(loop); }
		void SetPersistent(bool persistent) noexcept { mIsPersistent = persistent; }

		/// <summary>
		/// ミキシング
		/// </summary>
		void ApplyAndMix(int16_t* output, size_t framesRequested, uint16_t outputChannels, float masterVolume, float seVolume);
	private:
		AudioResource* mResource = nullptr;
		uint64_t mCurrentFrame = 0;

		std::atomic<float> mVolume{ 1.0f };
		std::atomic<bool>  mLoop{ false };
		std::atomic<bool>  mPlaying{ false };
		bool mIsPersistent = false; // シーン遷移でクリアするか
	};
}



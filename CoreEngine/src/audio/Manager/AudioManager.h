#pragma once

#include<Utility/Singleton/Singleton.hpp>

#include<vector>
#include<string>
#include<memory>
#include<atomic>
#include<mutex>

#include"miniaudio/miniaudio.h"

namespace audio
{
	class AudioResourceManager;
	class SoundEffect;

	class AudioManager : public utility::Singleton<AudioManager>
	{
	public:
		/// <summary>
		/// 初期化
		/// </summary>
		/// <returns>true:成功 false:失敗</returns>
		bool Initialize();

		// BGM制御
		void PlayBGM(const std::string& filePath, bool loop = true, float volume = 1.0f);
		void StopBGM();
		void PauseBGM();
		void ResumeBGM();

		// SE制御
		void PlaySE(const std::string& filePath,
			bool  loop = false,
			float volume = 1.0f,
			bool  persistent = false);

		void ClearSceneSounds();

		// ボリュームパス制御
		void SetMasterVolume(float volume) noexcept { mMasterVolume.store(volume); }
		void SetBgmVolume(float volume)    noexcept { mBgmVolume.store(volume); }
		void SetSeVolume(float volume)     noexcept { mSeVolume.store(volume); }

		[[nodiscard]] float MasterVolume() const noexcept { return mMasterVolume.load(); }
		[[nodiscard]] float BgmVolume()    const noexcept { return mBgmVolume.load(); }
		[[nodiscard]] float SeVolume()     const noexcept { return mSeVolume.load(); }

		static void DataCallback(ma_device* pDevice, void* pOutput,
			const void* pInput, ma_uint32 frameCount);


	private:
		void MixSounds(int16_t* output, size_t framesRequested, uint16_t channels);

		AudioResourceManager* mResources = nullptr;
		std::vector<SoundEffect>       mSeSounds;
		//std::unique_ptr<BgmStream> mActiveBgm = nullptr; // 単一アクティブBGMスロット
		std::recursive_mutex       mMtx;

		std::atomic<float> mMasterVolume{ 1.0f };
		std::atomic<float> mBgmVolume{ 1.0f };
		std::atomic<float> mSeVolume{ 1.0f };
	};
}



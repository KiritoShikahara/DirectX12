#pragma once

#include<Utility/Singleton/Singleton.hpp>

#include<vector>
#include<string>
#include<memory>
#include<atomic>
#include<mutex>

#include"miniaudio/miniaudio.h"
#include"../BGM/BGMStream.h"
#include"../SE/SoundEffect.h"

namespace audio
{
	class AudioResourceManager;

	class AudioManager : public utility::Singleton<AudioManager>
	{
		SINGLETON_CLASS(AudioManager);
	public:
		SINGLETON_ACCESSOR(AudioManager);

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

		///<summary>
		///BGMが現在アクティブか、Stop後や再生開始前はfalse
		///</summary>
		bool IsBgmPlaying();

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
		std::vector<SoundEffect> mSoundEffects;
		std::unique_ptr<BGMStream> mActiveBgm = nullptr; // 単一アクティブBGMスロット
		std::recursive_mutex mMtx;

		std::atomic<float> mMasterVolume{ 1.0f };
		std::atomic<float> mBgmVolume{ 1.0f };
		std::atomic<float> mSeVolume{ 1.0f };
	};
}

#define PLAY_BGM(filePath, ...) ::audio::AudioManager::Get().PlayBGM((filePath), ##__VA_ARGS__)

// BGM停止・一時停止・再開
#define STOP_BGM()   ::audio::AudioManager::Get().StopBGM()
#define PAUSE_BGM()  ::audio::AudioManager::Get().PauseBGM()
#define RESUME_BGM() ::audio::AudioManager::Get().ResumeBGM()

// SE再生
#define PLAY_SE(filePath, ...)  ::audio::AudioManager::Get().PlaySE((filePath), ##__VA_ARGS__)

// シーン切り替え時などのSE一括クリア
#define CLEAR_SE()   ::audio::AudioManager::Get().ClearSceneSounds()

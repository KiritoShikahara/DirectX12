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

		// 同一サウンドの同時再生数を制限しない場合に指定する
		static constexpr int32_t kUnlimitedInstances = -1;

		// SE制御
		// maxInstances: 同一ファイルの同時再生数上限。上限到達時は新規再生要求を無視する
		void PlaySE(const std::string& filePath,
			bool    loop = false,
			float   volume = 1.0f,
			bool    persistent = false,
			int32_t maxInstances = kUnlimitedInstances);

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
		void MixSounds(int16_t* output, size_t framesRequested, uint16_t channels, uint32_t sampleRate);

		/// <summary>
		/// float合成バス(mMixBuffer)へリミッターをかけながらint16へ書き出す。
		/// 単独音源のピークはしきい値を超えないためゲイン1.0のまま(=単独再生時と同じ音量)、
		/// 複数音源が重なってしきい値を超えたときだけアタック/リリースで滑らかにゲインを下げ、
		/// 1音ずつクリップする場合の硬い歪みを防ぐ
		/// </summary>
		void ApplyLimiterAndWrite(int16_t* output, size_t frameCount, uint16_t channels, uint32_t sampleRate);

		// SE同時発音数の全体上限。異なる種類のSEが重なるケースの安全弁
		static constexpr size_t kMaxTotalVoices = 32;

		// リミッターのしきい値(int16フルスケール直下。単独音源が通常これを超えないマージン)
		static constexpr float kLimiterThreshold = 32000.0f;

		// リミッターのアタック/リリース時定数(秒)。アタックは重なった瞬間の歪みを潰すため速く、
		// リリースはゲイン復帰時の耳障りな"ポンピング"を避けるためゆっくり
		static constexpr float kLimiterAttackSeconds = 0.005f;
		static constexpr float kLimiterReleaseSeconds = 0.15f;

		AudioResourceManager* mResources = nullptr;
		std::vector<SoundEffect> mSoundEffects;
		std::unique_ptr<BGMStream> mActiveBgm = nullptr; // 単一アクティブBGMスロット
		std::recursive_mutex mMtx;

		std::atomic<float> mMasterVolume{ 1.0f };
		std::atomic<float> mBgmVolume{ 1.0f };
		std::atomic<float> mSeVolume{ 1.0f };

		// 全音源をクリップせず合成するためのfloatバス(コールバックスレッド専有、毎フレーム再生成しない)
		std::vector<float> mMixBuffer;

		// リミッターの現在ゲイン。コールバックをまたいで保持し、滑らかに追従させる
		float mLimiterGain = 1.0f;
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

#include "pch.h"
#include "AudioManager.h"

#define MINIAUDIO_IMPLEMENTATION
#include"miniaudio/miniaudio.h"

#include"../Resource/AudioResource.h"
#include"../Resource/AudioResourceManager.h"

namespace audio
{
	bool AudioManager::Initialize()
	{
		mResources = &AudioResourceManager::Get();
		if (mResources == nullptr)
		{
			return false;
		}
		return true;
	}

	void AudioManager::PlayBGM(const std::string& filePath, bool loop, float volume)
	{
		auto bgm = std::make_unique<BGMStream>(filePath);
		if (!bgm->Open())
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Failed to open BGM file: ", filePath);
			return;
		}

		bgm->SetLoop(loop);
		bgm->SetVolume(volume);
		bgm->Play();

		std::lock_guard lock(mMtx);
		mActiveBgm = std::move(bgm); // 古いBGMはここで自動安全破棄されます
		DEBUG_LOG(sys::eLogLevel::Log, "Playing BGM: ", filePath);
	}

	void AudioManager::StopBGM()
	{
		std::lock_guard lock(mMtx);
		if (mActiveBgm)
		{
			mActiveBgm->Stop();
			mActiveBgm.reset();
		}
	}

	void AudioManager::PauseBGM()
	{
		std::lock_guard lock(mMtx);
		if (mActiveBgm)
		{
			mActiveBgm->Pause();
		}
	}

	void AudioManager::ResumeBGM()
	{
		std::lock_guard lock(mMtx);
		if (mActiveBgm)
		{
			mActiveBgm->Play();
		}
	}

	void AudioManager::PlaySE(const std::string& filePath, bool loop, float volume, bool persistent, int32_t maxInstances)
	{
		AudioResource* resource = mResources->GetResource(filePath);
		if (resource == nullptr)
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Failed to load SE resource: ", filePath);
			return;
		}

		std::lock_guard lock(mMtx);

		// 同一サウンドの同時再生数を制限する。上限に達している場合は新規再生を要求ごと無視する。
		// これにより同時ヒット・同時撃破時に同位相の波形が重なってクリッピング(ブー音)するのを防ぐ
		if (maxInstances != kUnlimitedInstances)
		{
			int32_t activeCount = 0;
			for (const auto& se : mSoundEffects)
			{
				if (se.Resource() == resource && se.IsPlaying())
				{
					++activeCount;
				}
			}

			if (activeCount >= maxInstances)
			{
				return;
			}
		}

		// 全体の同時発音数上限に達している場合、最も古い非persistentのSEを止めて枠を空ける
		if (mSoundEffects.size() >= kMaxTotalVoices)
		{
			auto it = std::find_if(mSoundEffects.begin(), mSoundEffects.end(),
				[](const SoundEffect& s) { return !s.IsPersistent(); });
			if (it == mSoundEffects.end())
			{
				DEBUG_LOG(sys::eLogLevel::Warning, "SE voice limit reached, dropping: ", filePath);
				return;
			}
			mSoundEffects.erase(it);
		}

		SoundEffect se(resource);
		se.SetLoop(loop);
		se.SetVolume(volume);
		se.SetPersistent(persistent);
		se.Play();
		mSoundEffects.push_back(std::move(se));
	}

	void AudioManager::ClearSceneSounds()
	{
		std::lock_guard lock(mMtx);
		std::erase_if(mSoundEffects, [](const SoundEffect& s) { return !s.IsPersistent(); });
	}

	bool AudioManager::IsBgmPlaying()
	{
		std::lock_guard lock(mMtx);
		return mActiveBgm != nullptr;
	}

	void AudioManager::DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
	{
		auto* mgr = reinterpret_cast<AudioManager*>(pDevice->pUserData);
		const uint16_t channels = static_cast<uint16_t>(pDevice->playback.channels);
		mgr->MixSounds(reinterpret_cast<int16_t*>(pOutput), frameCount, channels, pDevice->sampleRate);
	}
	void AudioManager::MixSounds(int16_t* output, size_t framesRequested, uint16_t channels, uint32_t sampleRate)
	{
		std::lock_guard lock(mMtx);

		// float合成バスを必要サイズだけ確保(拡張が要る場合のみ再確保、通常フレームは再利用)
		const size_t sampleCount = framesRequested * channels;
		if (mMixBuffer.size() < sampleCount)
		{
			mMixBuffer.resize(sampleCount);
		}
		std::fill(mMixBuffer.begin(), mMixBuffer.begin() + sampleCount, 0.0f);

		const float master = mMasterVolume.load();
		const float bgmVol = mBgmVolume.load();
		const float seVol = mSeVolume.load();

		// BGM のミキシング(floatバスへ加算するだけ、クリップしない)
		if (mActiveBgm && mActiveBgm->IsPlaying())
		{
			mActiveBgm->ApplyAndMix(mMixBuffer.data(), framesRequested, channels, master, bgmVol);
		}

		// SEのミキシングとライフサイクル管理(floatバスへ加算するだけ、クリップしない)
		for (auto it = mSoundEffects.begin(); it != mSoundEffects.end(); )
		{
			if (!it->IsPlaying())
			{
				it = mSoundEffects.erase(it);
				continue;
			}
			it->ApplyAndMix(mMixBuffer.data(), framesRequested, channels, master, seVol);
			++it;
		}

		// 合成済みのfloatバスへ1回だけリミッターをかけてint16へ書き出す
		ApplyLimiterAndWrite(output, framesRequested, channels, sampleRate);
	}

	void AudioManager::ApplyLimiterAndWrite(int16_t* output, size_t frameCount, uint16_t channels, uint32_t sampleRate)
	{
		// サンプルごとの追従係数(one-pole envelope)。時定数を秒からサンプル単位へ変換
		const float attackCoeff = std::exp(-1.0f / (kLimiterAttackSeconds * static_cast<float>(sampleRate)));
		const float releaseCoeff = std::exp(-1.0f / (kLimiterReleaseSeconds * static_cast<float>(sampleRate)));

		float gain = mLimiterGain;

		for (size_t frame = 0; frame < frameCount; ++frame)
		{
			// フレーム内の最大絶対値(全チャンネル共通ゲインでLR間の定位ズレを防ぐ)
			float peak = 0.0f;
			for (uint16_t ch = 0; ch < channels; ++ch)
			{
				peak = std::max(peak, std::abs(mMixBuffer[frame * channels + ch]));
			}

			// しきい値を超えた分だけゲインを下げる。超えていなければ目標は1.0(=無加工)
			const float targetGain = (peak > kLimiterThreshold) ? (kLimiterThreshold / peak) : 1.0f;
			const float coeff = (targetGain < gain) ? attackCoeff : releaseCoeff;
			gain = targetGain + (gain - targetGain) * coeff;

			for (uint16_t ch = 0; ch < channels; ++ch)
			{
				const float sample = mMixBuffer[frame * channels + ch] * gain;
				output[frame * channels + ch] = static_cast<int16_t>(
					std::clamp(sample, static_cast<float>(INT16_MIN), static_cast<float>(INT16_MAX)));
			}
		}

		mLimiterGain = gain;
	}
}
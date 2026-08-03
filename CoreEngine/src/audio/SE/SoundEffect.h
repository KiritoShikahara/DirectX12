#pragma once

#include <cstdint>
#include <atomic>

namespace audio
{
	struct AudioResource;

	class SoundEffect
	{
	public:
		// �R�s�[�͋֎~
		SoundEffect(const SoundEffect&) = delete;
		SoundEffect& operator=(const SoundEffect&) = delete;

		// �ړ����J�X�^����`����i= default ���O���j
		SoundEffect(SoundEffect&& other) noexcept;
		SoundEffect& operator=(SoundEffect&& other) noexcept;

		explicit SoundEffect(AudioResource* resource);
		~SoundEffect() = default;

		// �Đ��E��~�E�ꎞ��~
		void Play()  noexcept { mPlaying.store(true); }
		void Stop()  noexcept { mPlaying.store(false); mCurrentFrame = 0; }
		void Pause() noexcept { mPlaying.store(false); }

		// �A�N�Z�T
		[[nodiscard]] bool  IsPlaying()    const noexcept { return mPlaying.load(); }
		[[nodiscard]] bool  IsPersistent() const noexcept { return mIsPersistent; }
		[[nodiscard]] float Volume()       const noexcept { return mVolume.load(); }
		[[nodiscard]] bool  IsLoop()       const noexcept { return mLoop.load(); }

		// ����T�E���h���ǂ����̎��ʂɎg���iAudioResourceManager���p�X���ƂɃL���b�V�������|�C���^�j
		[[nodiscard]] const AudioResource* Resource() const noexcept { return mResource; }

		void SetVolume(float volume)        noexcept { mVolume.store(volume); }
		void SetLoop(bool loop)             noexcept { mLoop.store(loop); }
		void SetPersistent(bool persistent) noexcept { mIsPersistent = persistent; }

		/// <summary>
		/// ミキシング。クリップはしない(複数音源合成後にAudioManager側で一括してリミッターをかけるため)
		/// </summary>
		void ApplyAndMix(float* output, size_t framesRequested, uint16_t outputChannels, float masterVolume, float seVolume);
	private:
		AudioResource* mResource = nullptr;
		uint64_t mCurrentFrame = 0;

		std::atomic<float> mVolume{ 1.0f };
		std::atomic<bool>  mLoop{ false };
		std::atomic<bool>  mPlaying{ false };
		bool mIsPersistent = false; // �V�[���J�ڂŃN���A���邩
	};
}

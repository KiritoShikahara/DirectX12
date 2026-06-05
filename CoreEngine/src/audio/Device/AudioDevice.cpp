#include "pch.h"
#include "AudioDevice.h"

#include"../Manager/AudioManager.h"

namespace audio
{
	bool AudioDevice::Initialize(
		AudioManager* manager, uint32_t sampleRate, uint16_t channels)
	{
		if (manager)
		{
			return false;
		}

		mAudioManager = manager;

		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_s16;
		config.playback.channels = channels;
		config.sampleRate = sampleRate;
		config.dataCallback = AudioManager::DataCallback;
		config.pUserData = manager;

		if (ma_device_init(nullptr, &config, &mDevice) != MA_SUCCESS)
		{
			return false;
		}

		if (ma_device_start(&mDevice) != MA_SUCCESS)
		{
			ma_device_uninit(&mDevice);
			return false;
		}

		return true;
	}

	void AudioDevice::Finalize()
	{
		ma_device_uninit(&mDevice);
	}
}
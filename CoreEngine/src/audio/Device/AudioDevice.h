#pragma once



#include<Utility/Singleton/Singleton.hpp>
#include<miniaudio/miniaudio.h>
#include<cstdint>

namespace audio
{
    class AudioManager;

    /// <summary>
	/// ma_deviceÇä«óùÇ∑ÇÈÉNÉâÉX
    /// </summary>
    class AudioDevice : public utility::Singleton<AudioDevice>
    {
        SINGLETON_CLASS(AudioDevice);
    public:
        SINGLETON_ACCESSOR(AudioDevice);

        bool Initialize(AudioManager* manager,
            uint32_t sampleRate = 48000,
            uint16_t channels = 2);

		void Finalize();

    private:
        ma_device     mDevice = {};
        AudioManager* mAudioManager = nullptr;
    };

}



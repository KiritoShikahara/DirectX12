#pragma once
#include<cstdint>

namespace audio
{
#pragma pack(push, 1)
    struct AudioHeader
    {
        uint32_t SampleRate;
        uint16_t Channels;
        uint16_t BitsPerSample;
        uint64_t FrameCount;
    };
#pragma pack(pop)
}
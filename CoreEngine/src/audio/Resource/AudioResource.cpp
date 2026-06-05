#include"pch.h"
#include "AudioResource.h"

#include"../Data/AudioHeader.h"

#define MAX_AUDIO_SIZE (128 * 1024 * 1024) // 128MB

namespace audio
{
	bool AudioResource::LoadFromAud(const fs::path& path)
	{
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) return false;

        // ファイルサイズを取得して、ヘッダーに記載されたデータ量との整合性をチェック
        ifs.seekg(0, std::ios::end);
        const std::streamsize fileSize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        if (fileSize < static_cast<std::streamsize>(sizeof(AudioHeader)))
        {
            return false;
        }

        // データ読み込み
        AudioHeader header = {};
        ifs.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!ifs) return false;

        SampleRate = header.SampleRate;
        Channels = header.Channels;

        if (Channels == 0 || header.FrameCount == 0) return false;

        const uint64_t expectedDataBytes = header.FrameCount * Channels * sizeof(int16_t);

        if (fileSize < static_cast<std::streamsize>(sizeof(AudioHeader) + expectedDataBytes))
        {
            return false;
        }

        // SEでしか使用しないので最大容量に制限する。
		// 128mb以上のSEはさすがにないと思うので、これ以上は読み込まないように。
        const size_t maxAllowedElements = MAX_AUDIO_SIZE;
        const size_t requiredElements = static_cast<size_t>(header.FrameCount) * Channels;
        if (requiredElements > maxAllowedElements)
        {
            return false;
        }

        PcmData.resize(requiredElements);
        ifs.read(reinterpret_cast<char*>(PcmData.data()),
            static_cast<std::streamsize>(expectedDataBytes));

        return ifs.good() || ifs.eof();
	}
}
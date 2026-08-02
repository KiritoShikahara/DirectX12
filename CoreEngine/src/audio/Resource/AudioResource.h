#pragma once

#include<vector>
#include<filesystem>
#include<cstdint>

namespace fs = std::filesystem;

namespace audio	
{
	/// <summary>
	/// オーディオリソース構造体
    /// SEはリソースを参照して再生をして、BGMは自身でファイルから分割ロードをするようにする。
	/// </summary>
	struct AudioResource
	{
        uint32_t             SampleRate = 0;
        uint16_t             Channels = 0;
        std::vector<int16_t> PcmData;


        /// <summary>
        /// .aud ファイルを読み込み、メンバを初期化する
        /// </summary>
        bool LoadFromAud(const fs::path& path);

        [[nodiscard]] size_t FrameCount() const noexcept
        {
            if (Channels == 0) return 0;
            return PcmData.size() / Channels;
        }
    };
}



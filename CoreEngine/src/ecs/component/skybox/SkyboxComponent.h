#pragma once

#include <Utility/Export/Export.h>
#include <filesystem>

namespace ecs
{
    // スカイボックスコンポーネント
    struct ENGINE_API SkyboxComponent
    {
        // キューブマップテクスチャのファイルパス
        std::filesystem::path TexturePath;

        // 優先度
        int Priority = 0;

        // 影響度
        float Weight = 1.0f;
    };
}
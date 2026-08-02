#pragma once

#include <Utility/Export/Export.h>
#include <string>

namespace ecs
{
    /// <summary>
    /// アセットのキーを保持するコンポーネント
    /// </summary>
    struct ENGINE_API AssetKeyComponent
    {
        /// <summary>アセットのキー</summary>
        std::string Key;
    };
}
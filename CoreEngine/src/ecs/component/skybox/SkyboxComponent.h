#pragma once

#include<Utility/Export/Export.h>
#include<filesystem>

namespace ecs
{
    struct ENGINE_API SkyboxComponent
    {
        /// <summary>
        /// キューブマップテクスチャのファイルパス。
        /// DDSキューブマップを推奨。TextureManager::GetOrLoad() に渡される。
        /// </summary>
        std::filesystem::path TexturePath;

        /// <summary>
        /// 優先度。値が大きいほど「ベース（A側）」として扱われる。
        /// 同値の場合は不定順。
        /// </summary>
        int Priority = 0;

        /// <summary>
        /// 影響度 [0.0, 1.0]。
        /// このコンポーネントが B 側（ブレンド先）になった場合に使われるウェイト。
        /// A 側として選ばれた場合、このウェイトは無視される（常に 1.0 として扱う）。
        /// ゲームロジック側で自由に書き換えてよい。
        /// </summary>
        float Weight = 1.0f;
    };
}
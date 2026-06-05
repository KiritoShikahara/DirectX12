#pragma once

#include<Utility/Export/Export.h>

namespace graphics { class FbxResource; }

namespace ecs
{
    struct ENGINE_API FbxComponent
    {
        /// <summary>ロード済み FbxResource への参照</summary>
        graphics::FbxResource* Resource = nullptr;

        /// <summary>描画レイヤー (昇順でソートされる)</summary>
        int Layer = 0;

        /// <summary>false にすると UpdateAndDraw で無視される</summary>
        bool IsVisible = true;
    };
}
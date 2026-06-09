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

        /// <summary>
        /// 基準点を自動で足元に設定するかどうか true:する false:しない
        /// </summary>
        bool AutoPivot = true;

        /// <summary>
        /// ローカルの基準点オフセット
        /// </summary>
        DirectX::XMFLOAT3 PivotOffset = { 0.f, 0.f, 0.f };

        /// <summary>
        /// マテリアルの乗算するカラー
        /// </summary>
        DirectX::XMFLOAT4 CustomColor = { 1.f, 1.f, 1.f, 1.f };
    };
}
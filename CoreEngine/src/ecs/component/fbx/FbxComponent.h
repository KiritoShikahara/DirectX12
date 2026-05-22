#pragma once

#include<graphics/Color/Color.h>

namespace graphics
{
	class FbxResource;

}

namespace ecs
{
    struct ENGINE_API FbxModel
    {
        /// <summary>
        /// 共有リソースへの参照（非所有ポインタ）。
        /// FbxResourceManager が生存保証を担う。
        /// nullptr の場合は描画しない。
        /// </summary>
        graphics::FbxResource* Resource = nullptr;

        /// <summary>乗算カラー（マテリアルの BaseColor に掛け合わせる）</summary>
        graphics::Color Color = graphics::Color::White;

        /// <summary>輝度倍率（1.0 = 等倍）</summary>
        float Intensity = 1.f;

        /// <summary>描画順序。値が小さいほど先に描画される（2D レイヤーと共通）</summary>
        int Layer = 0;

        /// <summary>true のとき描画される</summary>
        bool IsVisible = true;

        /// <summary>リソースを設定するコンストラクタ</summary>
        explicit FbxModel(graphics::FbxResource* resource) : Resource(resource) {}
        FbxModel() = default;
    };


}
#pragma once

#include<Utility/Export/Export.h>
#include<graphics/Color/Color.h>

namespace graphics
{
	class ModelResource;
}

namespace ecs
{
    struct ENGINE_API Model
    {
        /// <summary>
        /// モデルリソースへの参照 (非所有ポインタ)
        /// ModelResourceManager が寿命を管理する
        /// nullptr の場合は描画しない
        /// </summary>
        graphics::ModelResource* Resource = nullptr;

        /// <summary>乗算カラー (マテリアルの BaseColor に掛け合わせる)</summary>
        graphics::Color Color = graphics::Color::White;

        /// <summary>輝度倍率 (1.0 = 等倍)</summary>
        float Intensity = 1.f;

        /// <summary>
        /// 描画順
        /// Sprite の Layer と共通の概念
        /// </summary>
        int Layer = 0;

        /// <summary>true のとき描画される</summary>
        bool IsVisible = true;

        explicit Model(graphics::ModelResource* resource) : Resource(resource) {}
        Model() = default;
    };
}
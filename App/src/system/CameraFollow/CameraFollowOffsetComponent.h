#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace ecs
{
    struct ENGINE_API CameraFollowOffsetComponent
    {
        /// <summary>繧ｫ繝｡繝ｩ菴咲ｽｮ縺ｮ繝励Ξ繧､繝､繝ｼ蠎ｧ讓吶°繧峨・縺壹ｉ縺鈴㍼</summary>
        DirectX::XMFLOAT3 Offset = { 0.f, 5.f, -10.f };

        /// <summary>豕ｨ隕也せ縺ｮ繝励Ξ繧､繝､繝ｼ蠎ｧ讓吶°繧峨・縺壹ｉ縺鈴㍼・井ｾ具ｼ夐ｭ縺ｮ鬮倥＆繧定ｦ九ｋ・・/summary>
        DirectX::XMFLOAT3 LookAtOffset = { 0.f, 1.5f, 0.f };
    };
}
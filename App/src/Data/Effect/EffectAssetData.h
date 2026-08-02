#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{

    struct EffectAssetData
    {
        int         Id = 0;
        std::string Path;

        REFLECT_BEGIN(EffectAssetData, "effect_assets")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Path)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::EffectAssetData);

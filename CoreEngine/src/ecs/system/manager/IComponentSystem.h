#pragma once

#include <entt/entt.hpp>
#include <Utility/Export/Export.h>

namespace ecs
{
    class ENGINE_API IUserSystem
    {
    public:
        virtual ~IUserSystem() = default;

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <param name="registry">ECSレジストリ</param>
        /// <param name="deltaTime">前フレームからの経過時間(秒)</param>
        virtual void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) = 0;
    };
}
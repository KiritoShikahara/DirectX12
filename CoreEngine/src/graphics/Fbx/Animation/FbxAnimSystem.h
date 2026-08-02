#pragma once
#include <entt/entt.hpp>

namespace graphics
{

    class FbxAnimSystem
    {
    public:
        /// <summary>
        /// FbxComponent + FbxAnimComponent を持つエンティティの
        /// アニメーション時間を deltaTime 秒進める
        /// </summary>
        static void Update(entt::registry& registry, float deltaTime);
    };

} // namespace graphics
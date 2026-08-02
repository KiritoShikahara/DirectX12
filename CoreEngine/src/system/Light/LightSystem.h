#pragma once
#include <entt/entt.hpp>

namespace sys
{
    /// <summary>
    /// ライトの更新管理
    /// </summary>
    class LightSystem
    {
    public:
        LightSystem() = delete;
        ~LightSystem() = delete;

        /// <summary>
        /// FbxRenderer::Begin() の前に呼ぶこと
        /// TransformコンポーネントからPosition/Directionを自動取得する
        /// </summary>
        static void Update(entt::registry& registry);

        /// <summary>
        /// ディレクションライトの数値変更をImGUiでおこうなう
        /// </summary>
        /// <param name="registry"></param>
        static void DebugUI(entt::registry& registry);
    };
}



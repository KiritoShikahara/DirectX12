#pragma once
#include <entt/entt.hpp>

namespace graphics
{
    // ============================================================
    //  FbxAnimSystem  (ModelAnimSystem 相当)
    //  FbxComponent + FbxAnimComponent を持つ全エンティティの
    //  アニメーション時間を進める
    //
    //  呼び出し順:
    //    1. FbxAnimSystem::Update(registry, deltaTime)  ← 時間進行
    //    2. FbxRenderer::Get().Begin()
    //    3. FbxRenderer::Get().UpdateAndDraw(registry) ← CalcBoneMatrices を内部で呼ぶ
    //    4. FbxRenderer::Get().End(cmdList)
    // ============================================================
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
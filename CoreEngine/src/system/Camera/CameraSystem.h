#pragma once

#include <entt/entt.hpp>
#include <Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>
#include<graphics/Model/ModelData.h>

namespace ecs
{
    struct CameraComponent;
    struct Transform;
}

namespace sys
{

    /// <summary>
    /// カメラのシステム
    /// </summary>
    class ENGINE_API CameraSystem : public utility::Singleton<CameraSystem>
    {
        SINGLETON_CLASS(CameraSystem);
    public:
        SINGLETON_ACCESSOR(CameraSystem);
        
        /// <summary>
        /// 初期化
        /// </summary>
        /// <returns></returns>
        bool Initialize();

        /// <summary>
        /// フレーム先頭で呼ぶ。
        /// キャッシュが無効なら再検索し行列を更新する。
        /// </summary>
        void Update(entt::registry& registry);

        bool HasMainCamera() const { return mHasMainCamera; }

        /// <summary>GPU 転送用カメラデータ（HasMainCamera() == true のとき有効）</summary>
        const graphics::ModelCameraData& GetShaderData() const { return mShaderData; }

        entt::entity GetMainCameraEntity() const { return mMainCameraEntity; }

        /// <summary>プログラムからメインカメラを切り替える</summary>
        void SetMainCameraEntity(entt::registry& registry, entt::entity entity);

        /// <summary>カメラをImGuiから操作できるようにする。</summary>
        void ImGuiUpdate(entt::registry& registry);

    private:
        void SearchMainCamera(entt::registry& registry);
        void UpdateMatrices(entt::registry& registry);

        entt::entity               mMainCameraEntity = entt::null;
        graphics::ModelCameraData mShaderData = {};
        bool                       mHasMainCamera = false;
    };
}
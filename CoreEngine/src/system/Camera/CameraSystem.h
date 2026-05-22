#pragma once

#include <entt/entt.hpp>
#include <Utility/Singleton/Singleton.hpp>
#include<graphics/Fbx/Resouce/FbxData.h>
#include<Utility/Export/Export.h>

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
        /// フレーム先頭で呼ぶ。
        /// キャッシュが無効なら再検索し行列を更新する。
        /// </summary>
        void Update(entt::registry& registry);

        bool HasMainCamera() const { return mHasMainCamera; }

        /// <summary>GPU 転送用カメラデータ（HasMainCamera() == true のとき有効）</summary>
        const graphics::CameraShaderData& GetShaderData() const { return mShaderData; }

        entt::entity GetMainCameraEntity() const { return mMainCameraEntity; }

        /// <summary>プログラムからメインカメラを切り替える</summary>
        void SetMainCameraEntity(entt::registry& registry, entt::entity entity);

    private:
        void SearchMainCamera(entt::registry& registry);
        void UpdateMatrices(entt::registry& registry);

        entt::entity               mMainCameraEntity = entt::null;
        graphics::CameraShaderData mShaderData = {};
        bool                       mHasMainCamera = false;
    };
}
#pragma once

#include <entt/entt.hpp>
#include <Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>
#include<graphics/Fbx/Data/FbxData.h>

namespace ecs
{
    struct CameraComponent;
    struct Transform;
}

namespace sys
{
    /// <summary>レイ（始点 + 正規化方向）</summary>
    struct ENGINE_API Ray
    {
        DirectX::XMFLOAT3 Origin = { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT3 Direction = { 0.f, 0.f, 1.f };
    };

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
        const graphics::FbxCameraData& GetShaderData() const { return mShaderData; }

        entt::entity GetMainCameraEntity() const { return mMainCameraEntity; }

        /// <summary>プログラムからメインカメラを切り替える</summary>
        void SetMainCameraEntity(entt::registry& registry, entt::entity entity);

        /// <summary>カメラをImGuiから操作できるようにする。</summary>
        void ImGuiUpdate(entt::registry& registry);

        /// <summary>
        /// スクリーン座標（Window の仮想解像度基準、左上原点）からワールド空間のレイを生成する。
        /// メインカメラが存在しない場合は無効なレイ（Origin=0, Direction=(0,0,1)）を返す。
        /// </summary>
        Ray ScreenPointToRay(entt::registry& registry, const DirectX::XMFLOAT2& screenPos) const;

        /// <summary>
        /// スクリーン座標から、カメラ位置を起点に distance だけレイ方向へ進んだワールド座標を取得する。
        /// </summary>
        DirectX::XMFLOAT3 ScreenPointToWorld(entt::registry& registry, const DirectX::XMFLOAT2& screenPos, float distance) const;

        /// <summary>
        /// スクリーン座標から、ワールド空間の Y = planeY 平面とレイの交点を求める。
        /// </summary>
        /// <returns>true:交差した（outWorldPos に結果を格納） false:平行・後方などで交差しない</returns>
        bool ScreenPointToWorldOnPlaneY(entt::registry& registry, const DirectX::XMFLOAT2& screenPos, float planeY, DirectX::XMFLOAT3& outWorldPos) const;

    private:
        void SearchMainCamera(entt::registry& registry);
        void UpdateMatrices(entt::registry& registry);

        entt::entity               mMainCameraEntity = entt::null;
        graphics::FbxCameraData mShaderData = {};
        bool                       mHasMainCamera = false;
    };
}
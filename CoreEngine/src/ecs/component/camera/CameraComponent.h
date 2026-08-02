#pragma once

#include <DirectXMath.h>
#include <Utility/Export/Export.h>

namespace sys { class Window; }

namespace ecs
{
    struct Transform;

    /// <summary>
    /// カメラコンポーネント
    /// </summary>
    struct CameraComponent
    {
        /// <summary>垂直視野角（度）</summary>
        float Fov = 60.f;

        /// <summary>ニアクリップ面</summary>
        float Near = 0.1f;

        /// <summary>ファークリップ面</summary>
        float Far = 1000.f;

        /// <summary>アスペクト比（幅 / 高さ）</summary>
        float AspectRatio = 16.f / 9.f;

        /// <summary>このカメラをメインカメラとして使用するか</summary>
        bool IsMainCamera = false;

        /// <summary>ビュー行列</summary>
        DirectX::XMFLOAT4X4 ViewMatrix = {};
        /// <summary>プロジェクション行列</summary>
        DirectX::XMFLOAT4X4 ProjectionMatrix = {};
        /// <summary>ビュープロジェクション行列</summary>
        DirectX::XMFLOAT4X4 ViewProjectionMatrix = {};

        /// <summary>カメラワールド座標（シェーダへ渡す用）</summary>
        DirectX::XMFLOAT3   Position = {};

        /// <summary>Window のバーチャル解像度からアスペクト比を設定する</summary>
        void SetAspectRatioFromWindow(const sys::Window& window);

        /// <summary>Transform の位置・姿勢から行列を再計算する</summary>
        void UpdateMatrices(const Transform& transform);

        /// <summary>ビュー行列を取得する</summary>
        DirectX::XMMATRIX GetViewMatrix()           const;
        /// <summary>プロジェクション行列を取得する</summary>
        DirectX::XMMATRIX GetProjectionMatrix()     const;
        /// <summary>ビュープロジェクション行列を取得する</summary>
        DirectX::XMMATRIX GetViewProjectionMatrix() const;
    };
}
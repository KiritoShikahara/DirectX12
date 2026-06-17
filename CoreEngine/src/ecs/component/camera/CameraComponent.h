#pragma once

#include <DirectXMath.h>
#include<Utility/Export/Export.h>

namespace sys { class Window; }

namespace ecs
{
	struct Transform;

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

        /*
        * 計算済みの行列
        */
        DirectX::XMFLOAT4X4 ViewMatrix = {};
        DirectX::XMFLOAT4X4 ProjectionMatrix = {};
        DirectX::XMFLOAT4X4 ViewProjectionMatrix = {};

        /// <summary>カメラワールド座標（シェーダへ渡す用）</summary>
        DirectX::XMFLOAT3   Position = {};

        /*
        * 操作API
        */

        /// <summary>
        /// Window のバーチャル解像度からアスペクト比を設定するヘルパー。
        /// Initialize 後に一度呼ぶか、リサイズ時に呼ぶ。
        /// </summary>
        void SetAspectRatioFromWindow(const sys::Window& window);

        /// <summary>
        /// Transform の位置・姿勢から View / Projection / VP 行列を再計算。
        /// FbxRenderer::UpdateAndDraw() 内で自動的に呼ばれる。
        /// </summary>
        void UpdateMatrices(const Transform& transform);

        /*
        * 行列アクセサ
        */ 
        DirectX::XMMATRIX GetViewMatrix()           const;
        DirectX::XMMATRIX GetProjectionMatrix()     const;
        DirectX::XMMATRIX GetViewProjectionMatrix() const;
	};
}



#pragma once

#include <Utility/Export/Export.h>
#include <DirectXMath.h>
#include <cmath>

namespace ecs
{
    /// <summary>
    /// 2D 3D 共用トランスフォーム
    /// </summary>
    struct ENGINE_API Transform
    {
        Transform() = default;

        /// <summary>3D 用コンストラクタ</summary>
        Transform(
            DirectX::XMFLOAT3 position,
            DirectX::XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 1.f },
            DirectX::XMFLOAT3 scale = { 1.f, 1.f, 1.f });

        /// <summary>2D 用コンストラクタ</summary>
        Transform(
            DirectX::XMFLOAT2 position,
            float             rotationRad = 0.f,
            DirectX::XMFLOAT2 scale = { 1.f, 1.f });

        void SetPosition(DirectX::FXMVECTOR v);
        void SetPosition(const DirectX::XMFLOAT3& v);
        void SetPosition(float x, float y, float z);

        void SetRotation(DirectX::FXMVECTOR q);
        void SetRotation(const DirectX::XMFLOAT4& q);

        void SetScale(DirectX::FXMVECTOR v);
        void SetScale(const DirectX::XMFLOAT3& v);
        void SetScale(float uniform);

        void SetEulerAngles(float pitchRad, float yawRad, float rollRad);
        void SetEulerAnglesDeg(float pitchDeg, float yawDeg, float rollDeg);

        void Set2DPosition(DirectX::XMFLOAT2 v);
        void Set2DPosition(float x, float y);
        void Set2DRotation(float radians);
        void Set2DScale(DirectX::XMFLOAT2 v);

        void SetXPosition(float x);
        void SetYPosition(float y);
        void SetZPosition(float z);

        void Translate(DirectX::FXMVECTOR delta);
        void Translate(float dx, float dy, float dz = 0.f);
        void Rotate(DirectX::FXMVECTOR deltaQ);
        void Rotate2D(float radians);
        void ScaleBy(float factor);

        const DirectX::XMFLOAT3& GetPosition()  const { return mPosition; }
        const DirectX::XMFLOAT4& GetRotation()  const { return mRotation; }
        const DirectX::XMFLOAT3& GetScale()     const { return mScale; }

        DirectX::XMFLOAT2 Get2DPosition() const { return { mPosition.x, mPosition.y }; }
        float Get2DRotation() const;

        DirectX::XMVECTOR GetForward() const;
        DirectX::XMVECTOR GetBack()    const;
        DirectX::XMVECTOR GetUp()      const;
        DirectX::XMVECTOR GetDown()    const;
        DirectX::XMVECTOR GetRight()   const;
        DirectX::XMVECTOR GetLeft()    const;

        /// <summary>指定座標の方向を向くように回転を設定する</summary>
        void LookAt(DirectX::FXMVECTOR targetPosition);
        void LookAt(const DirectX::XMFLOAT3& targetPosition);
        void LookAt(float x, float y, float z);

        /// <summary>指定座標の方向を向くがピッチは変化させない</summary>
        void LookAtHorizontal(DirectX::FXMVECTOR targetPosition);
        void LookAtHorizontal(const DirectX::XMFLOAT3& targetPosition);
        void LookAtHorizontal(float x, float y, float z);

        /// <summary>指定座標へ向かう正規化済みの移動方向ベクトルを取得する</summary>
        DirectX::XMVECTOR GetDirectionTo(DirectX::FXMVECTOR targetPosition) const;
        DirectX::XMVECTOR GetDirectionTo(const DirectX::XMFLOAT3& targetPosition) const;
        DirectX::XMVECTOR GetDirectionTo(float x, float y, float z) const;

        const DirectX::XMMATRIX& GetWorldMatrix() const;
        DirectX::XMMATRIX Get2DWorldMatrix() const;

        void Reset();
        bool IsDirty() const { return mIsDirty; }

    private:
        void MarkDirty() const { mIsDirty = true; }

        DirectX::XMFLOAT3 mPosition = { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT4 mRotation = { 0.f, 0.f, 0.f, 1.f };
        DirectX::XMFLOAT3 mScale = { 1.f, 1.f, 1.f };

        mutable DirectX::XMMATRIX mCachedMatrix = DirectX::XMMatrixIdentity();
        mutable bool              mIsDirty = true;
    };
}
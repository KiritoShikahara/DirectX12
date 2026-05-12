#include"TransformComponent.h"

namespace ecs
{
    // ==============================================================
    //  コンストラクタ
    // ==============================================================

    Transform::Transform(
        DirectX::XMFLOAT3 position,
        DirectX::XMFLOAT4 rotation,
        DirectX::XMFLOAT3 scale)
        : mPosition(position), mRotation(rotation), mScale(scale)
    {
    }

    Transform::Transform(
        DirectX::XMFLOAT2 position,
        float             rotationRad,
        DirectX::XMFLOAT2 scale)
    {
        Set2DPosition(position);
        Set2DRotation(rotationRad);
        Set2DScale(scale);
    }

    // ==============================================================
    //  3D セッター
    // ==============================================================

    void Transform::SetPosition(DirectX::FXMVECTOR v)
    {
        DirectX::XMStoreFloat3(&mPosition, v);
        MarkDirty();
    }
    void Transform::SetPosition(const DirectX::XMFLOAT3& v) { mPosition = v; MarkDirty(); }
    void Transform::SetPosition(float x, float y, float z) { mPosition = { x, y, z }; MarkDirty(); }

    void Transform::SetRotation(DirectX::FXMVECTOR q)
    {
        DirectX::XMStoreFloat4(&mRotation, q);
        MarkDirty();
    }
    void Transform::SetRotation(const DirectX::XMFLOAT4& q) { mRotation = q; MarkDirty(); }

    void Transform::SetScale(DirectX::FXMVECTOR v)
    {
        DirectX::XMStoreFloat3(&mScale, v);
        MarkDirty();
    }
    void Transform::SetScale(const DirectX::XMFLOAT3& v) { mScale = v; MarkDirty(); }
    void Transform::SetScale(float uniform) { mScale = { uniform, uniform, uniform }; MarkDirty(); }

    // ==============================================================
    //  2D セッター
    // ==============================================================

    void Transform::Set2DPosition(DirectX::XMFLOAT2 v)
    {
        mPosition = { v.x, v.y, mPosition.z };
        MarkDirty();
    }
    void Transform::Set2DPosition(float x, float y)
    {
        mPosition = { x, y, mPosition.z };
        MarkDirty();
    }
    void Transform::Set2DRotation(float radians)
    {
        DirectX::XMStoreFloat4(&mRotation,
            DirectX::XMQuaternionRotationAxis(
                DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f), radians));
        MarkDirty();
    }
    void Transform::Set2DScale(DirectX::XMFLOAT2 v)
    {
        mScale = { v.x, v.y, mScale.z };
        MarkDirty();
    }

    // ==============================================================
    //  操作メソッド
    // ==============================================================

    void Transform::Translate(DirectX::FXMVECTOR delta)
    {
        DirectX::XMStoreFloat3(&mPosition,
            DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&mPosition), delta));
        MarkDirty();
    }
    void Transform::Translate(float dx, float dy, float dz)
    {
        Translate(DirectX::XMVectorSet(dx, dy, dz, 0.f));
    }
    void Transform::Rotate(DirectX::FXMVECTOR deltaQ)
    {
        DirectX::XMStoreFloat4(&mRotation,
            DirectX::XMQuaternionMultiply(
                DirectX::XMLoadFloat4(&mRotation), deltaQ));
        MarkDirty();
    }
    void Transform::Rotate2D(float radians)
    {
        Rotate(DirectX::XMQuaternionRotationAxis(
            DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f), radians));
    }
    void Transform::ScaleBy(float factor)
    {
        DirectX::XMStoreFloat3(&mScale,
            DirectX::XMVectorScale(DirectX::XMLoadFloat3(&mScale), factor));
        MarkDirty();
    }

    // ==============================================================
    //  ゲッター
    // ==============================================================

    float Transform::Get2DRotation() const
    {
        using namespace DirectX;
        const float& x = mRotation.x;
        const float& y = mRotation.y;
        const float& z = mRotation.z;
        const float& w = mRotation.w;
        return std::atan2(2.f * (w * z + x * y), 1.f - 2.f * (y * y + z * z));
    }

    // ==============================================================
    //  方向ベクトル
    // ==============================================================

    DirectX::XMVECTOR Transform::GetForward() const
    {
        return DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f),
            DirectX::XMLoadFloat4(&mRotation));
    }
    DirectX::XMVECTOR Transform::GetBack() const { return DirectX::XMVectorNegate(GetForward()); }
    DirectX::XMVECTOR Transform::GetUp() const
    {
        return DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f),
            DirectX::XMLoadFloat4(&mRotation));
    }
    DirectX::XMVECTOR Transform::GetDown() const { return DirectX::XMVectorNegate(GetUp()); }
    DirectX::XMVECTOR Transform::GetRight() const
    {
        return DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(1.f, 0.f, 0.f, 0.f),
            DirectX::XMLoadFloat4(&mRotation));
    }
    DirectX::XMVECTOR Transform::GetLeft() const { return DirectX::XMVectorNegate(GetRight()); }

    // ==============================================================
    //  ワールド行列
    // ==============================================================

    const DirectX::XMMATRIX& Transform::GetWorldMatrix() const
    {
        if (mIsDirty)
        {
            mCachedMatrix = DirectX::XMMatrixAffineTransformation(
                DirectX::XMLoadFloat3(&mScale),
                DirectX::XMVectorSet(0.f, 0.f, 0.f, 1.f),
                DirectX::XMLoadFloat4(&mRotation),
                DirectX::XMLoadFloat3(&mPosition));
            mIsDirty = false;
        }
        return mCachedMatrix;
    }

    DirectX::XMMATRIX Transform::Get2DWorldMatrix() const
    {
        using namespace DirectX;
        return XMMatrixAffineTransformation2D(
            XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&mScale)),
            XMVectorSet(0.f, 0.f, 0.f, 1.f),
            Get2DRotation(),
            XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&mPosition)));
    }

    // ==============================================================
    //  ユーティリティ
    // ==============================================================

    void Transform::Reset()
    {
        mPosition = { 0.f, 0.f, 0.f };
        mRotation = { 0.f, 0.f, 0.f, 1.f };
        mScale = { 1.f, 1.f, 1.f };
        MarkDirty();
    }

} // namespace ecs
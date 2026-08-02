#include "pch.h"
#include "TransformComponent.h"

namespace ecs
{
    // コンストラクタ
    Transform::Transform(
        DirectX::XMFLOAT3 position,
        DirectX::XMFLOAT4 rotation,
        DirectX::XMFLOAT3 scale)
        : mPosition(position), mRotation(rotation), mScale(scale)
    {
    }

    // 2D用コンストラクタ
    Transform::Transform(
        DirectX::XMFLOAT2 position,
        float             rotationRad,
        DirectX::XMFLOAT2 scale)
    {
        Set2DPosition(position);
        Set2DRotation(rotationRad);
        Set2DScale(scale);
    }

    // 位置を設定する
    void Transform::SetPosition(DirectX::FXMVECTOR v)
    {
        DirectX::XMStoreFloat3(&mPosition, v);
        MarkDirty();
    }

    // 位置を設定する
    void Transform::SetPosition(const DirectX::XMFLOAT3& v)
    {
        mPosition = v;
        MarkDirty();
    }

    // 位置を設定する
    void Transform::SetPosition(float x, float y, float z)
    {
        mPosition = { x, y, z };
        MarkDirty();
    }

    // 回転を設定する
    void Transform::SetRotation(DirectX::FXMVECTOR q)
    {
        DirectX::XMStoreFloat4(&mRotation, q);
        MarkDirty();
    }

    // 回転を設定する
    void Transform::SetRotation(const DirectX::XMFLOAT4& q)
    {
        mRotation = q;
        MarkDirty();
    }

    // スケールを設定する
    void Transform::SetScale(DirectX::FXMVECTOR v)
    {
        DirectX::XMStoreFloat3(&mScale, v);
        MarkDirty();
    }

    // スケールを設定する
    void Transform::SetScale(const DirectX::XMFLOAT3& v)
    {
        mScale = v;
        MarkDirty();
    }

    // スケールを設定する
    void Transform::SetScale(float uniform)
    {
        mScale = { uniform, uniform, uniform };
        MarkDirty();
    }

    // オイラー角で回転を設定する
    void Transform::SetEulerAngles(float pitchRad, float yawRad, float rollRad)
    {
        DirectX::XMStoreFloat4(&mRotation,
            DirectX::XMQuaternionRotationRollPitchYaw(pitchRad, yawRad, rollRad));
        MarkDirty();
    }

    // 度数法のオイラー角で回転を設定する
    void Transform::SetEulerAnglesDeg(float pitchDeg, float yawDeg, float rollDeg)
    {
        SetEulerAngles(
            DirectX::XMConvertToRadians(pitchDeg),
            DirectX::XMConvertToRadians(yawDeg),
            DirectX::XMConvertToRadians(rollDeg));
    }

    // 2D位置を設定する
    void Transform::Set2DPosition(DirectX::XMFLOAT2 v)
    {
        mPosition = { v.x, v.y, mPosition.z };
        MarkDirty();
    }

    // 2D位置を設定する
    void Transform::Set2DPosition(float x, float y)
    {
        mPosition = { x, y, mPosition.z };
        MarkDirty();
    }

    // 2D回転を設定する
    void Transform::Set2DRotation(float radians)
    {
        DirectX::XMStoreFloat4(&mRotation,
            DirectX::XMQuaternionRotationAxis(
                DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f), radians));
        MarkDirty();
    }

    // 2Dスケールを設定する
    void Transform::Set2DScale(DirectX::XMFLOAT2 v)
    {
        mScale = { v.x, v.y, mScale.z };
        MarkDirty();
    }

    // X座標を設定する
    void Transform::SetXPosition(float x)
    {
        mPosition.x = x;
        MarkDirty();
    }

    // Y座標を設定する
    void Transform::SetYPosition(float y)
    {
        mPosition.y = y;
        MarkDirty();
    }

    // Z座標を設定する
    void Transform::SetZPosition(float z)
    {
        mPosition.z = z;
        MarkDirty();
    }

    // 平行移動させる
    void Transform::Translate(DirectX::FXMVECTOR delta)
    {
        DirectX::XMStoreFloat3(&mPosition,
            DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&mPosition), delta));
        MarkDirty();
    }

    // 平行移動させる
    void Transform::Translate(float dx, float dy, float dz)
    {
        Translate(DirectX::XMVectorSet(dx, dy, dz, 0.f));
    }

    // 回転を加算する
    void Transform::Rotate(DirectX::FXMVECTOR deltaQ)
    {
        DirectX::XMStoreFloat4(&mRotation,
            DirectX::XMQuaternionMultiply(
                DirectX::XMLoadFloat4(&mRotation), deltaQ));
        MarkDirty();
    }

    // 2D回転を加算する
    void Transform::Rotate2D(float radians)
    {
        Rotate(DirectX::XMQuaternionRotationAxis(
            DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f), radians));
    }

    // スケールを乗算する
    void Transform::ScaleBy(float factor)
    {
        DirectX::XMStoreFloat3(&mScale,
            DirectX::XMVectorScale(DirectX::XMLoadFloat3(&mScale), factor));
        MarkDirty();
    }

    // 2D回転を取得する
    float Transform::Get2DRotation() const
    {
        using namespace DirectX;
        const float& x = mRotation.x;
        const float& y = mRotation.y;
        const float& z = mRotation.z;
        const float& w = mRotation.w;
        return std::atan2(2.f * (w * z + x * y), 1.f - 2.f * (y * y + z * z));
    }

    // 前方ベクトルを取得する
    DirectX::XMVECTOR Transform::GetForward() const
    {
        return DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f),
            DirectX::XMLoadFloat4(&mRotation));
    }

    // 後方ベクトルを取得する
    DirectX::XMVECTOR Transform::GetBack() const
    {
        return DirectX::XMVectorNegate(GetForward());
    }

    // 上方ベクトルを取得する
    DirectX::XMVECTOR Transform::GetUp() const
    {
        return DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f),
            DirectX::XMLoadFloat4(&mRotation));
    }

    // 下方ベクトルを取得する
    DirectX::XMVECTOR Transform::GetDown() const
    {
        return DirectX::XMVectorNegate(GetUp());
    }

    // 右方ベクトルを取得する
    DirectX::XMVECTOR Transform::GetRight() const
    {
        return DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(1.f, 0.f, 0.f, 0.f),
            DirectX::XMLoadFloat4(&mRotation));
    }

    // 左方ベクトルを取得する
    DirectX::XMVECTOR Transform::GetLeft() const
    {
        return DirectX::XMVectorNegate(GetRight());
    }

    // 指定座標を向くよう回転を設定する
    void Transform::LookAt(DirectX::FXMVECTOR targetPosition)
    {
        using namespace DirectX;

        XMVECTOR pos = XMLoadFloat3(&mPosition);
        XMVECTOR dir = XMVectorSubtract(targetPosition, pos);

        if (XMVectorGetX(XMVector3LengthSq(dir)) < 1e-8f)
        {
            return;
        }
        dir = XMVector3Normalize(dir);

        XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
        if (fabsf(XMVectorGetY(dir)) > 0.9999f)
        {
            up = XMVectorSet(0.f, 0.f, 1.f, 0.f);
        }

        XMMATRIX viewMat = XMMatrixLookToLH(XMVectorZero(), dir, up);
        XMMATRIX worldRot = XMMatrixTranspose(viewMat);

        SetRotation(XMQuaternionRotationMatrix(worldRot));
    }

    // 指定座標を向くよう回転を設定する
    void Transform::LookAt(const DirectX::XMFLOAT3& targetPosition)
    {
        LookAt(DirectX::XMLoadFloat3(&targetPosition));
    }

    // 指定座標を向くよう回転を設定する
    void Transform::LookAt(float x, float y, float z)
    {
        LookAt(DirectX::XMVectorSet(x, y, z, 0.f));
    }

    // 水平方向のみ指定座標を向くよう回転を設定する
    void Transform::LookAtHorizontal(DirectX::FXMVECTOR targetPosition)
    {
        using namespace DirectX;

        XMFLOAT3 target;
        XMStoreFloat3(&target, targetPosition);
        target.y = mPosition.y;

        LookAt(XMLoadFloat3(&target));
    }

    // 水平方向のみ指定座標を向くよう回転を設定する
    void Transform::LookAtHorizontal(const DirectX::XMFLOAT3& targetPosition)
    {
        LookAtHorizontal(DirectX::XMLoadFloat3(&targetPosition));
    }

    // 水平方向のみ指定座標を向くよう回転を設定する
    void Transform::LookAtHorizontal(float x, float y, float z)
    {
        LookAtHorizontal(DirectX::XMVectorSet(x, y, z, 0.f));
    }

    // 指定座標への方向ベクトルを取得する
    DirectX::XMVECTOR Transform::GetDirectionTo(DirectX::FXMVECTOR targetPosition) const
    {
        using namespace DirectX;

        XMVECTOR pos = XMLoadFloat3(&mPosition);
        XMVECTOR diff = XMVectorSubtract(targetPosition, pos);

        if (XMVectorGetX(XMVector3LengthSq(diff)) < 1e-8f)
        {
            return XMVectorZero();
        }
        return XMVector3Normalize(diff);
    }

    // 指定座標への方向ベクトルを取得する
    DirectX::XMVECTOR Transform::GetDirectionTo(const DirectX::XMFLOAT3& targetPosition) const
    {
        return GetDirectionTo(DirectX::XMLoadFloat3(&targetPosition));
    }

    // 指定座標への方向ベクトルを取得する
    DirectX::XMVECTOR Transform::GetDirectionTo(float x, float y, float z) const
    {
        return GetDirectionTo(DirectX::XMVectorSet(x, y, z, 0.f));
    }

    // ワールド行列を取得する
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

    // 2Dワールド行列を取得する
    DirectX::XMMATRIX Transform::Get2DWorldMatrix() const
    {
        using namespace DirectX;
        return XMMatrixAffineTransformation2D(
            XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&mScale)),
            XMVectorSet(0.f, 0.f, 0.f, 1.f),
            Get2DRotation(),
            XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&mPosition)));
    }

    // トランスフォームを初期状態にリセットする
    void Transform::Reset()
    {
        mPosition = { 0.f, 0.f, 0.f };
        mRotation = { 0.f, 0.f, 0.f, 1.f };
        mScale = { 1.f, 1.f, 1.f };
        MarkDirty();
    }
}
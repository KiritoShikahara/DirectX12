#pragma once
#include <Utility/Export/Export.h>
#include <DirectXMath.h>
#include <cmath>

namespace ecs
{
    /// <summary>
    /// 2D・3D 共用トランスフォーム。
    ///
    /// 内部は常に 3D（Position/Rotation/Scale）で保持する。
    /// 2D として使う場合は Z=0・回転はZ軸クォータニオンに変換して格納する。
    ///
    /// ワールド行列はダーティフラグで管理し、
    /// 変更があったフレームのみ再計算する（毎フレーム呼んでも安全）。
    /// </summary>
    struct ENGINE_API Transform
    {
        // ==============================================================
        //  コンストラクタ
        // ==============================================================

        Transform() = default;

        /// <summary>3D 用コンストラクタ</summary>
        Transform(
            DirectX::XMFLOAT3 position,
            DirectX::XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 1.f },
            DirectX::XMFLOAT3 scale = { 1.f, 1.f, 1.f });

        /// <summary>2D 用コンストラクタ（Z=0・回転はZ軸）</summary>
        Transform(
            DirectX::XMFLOAT2 position,
            float             rotationRad = 0.f,
            DirectX::XMFLOAT2 scale = { 1.f, 1.f });

        // ==============================================================
        //  3D セッター
        // ==============================================================

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
        // ==============================================================
        //  2D セッター
        // ==============================================================

        void Set2DPosition(DirectX::XMFLOAT2 v);
        void Set2DPosition(float x, float y);
        void Set2DRotation(float radians);
        void Set2DScale(DirectX::XMFLOAT2 v);

        // ==============================================================
        //  操作メソッド
        // ==============================================================

        void Translate(DirectX::FXMVECTOR delta);
        void Translate(float dx, float dy, float dz = 0.f);
        void Rotate(DirectX::FXMVECTOR deltaQ);
        void Rotate2D(float radians);
        void ScaleBy(float factor);

        // ==============================================================
        //  ゲッター
        // ==============================================================

        const DirectX::XMFLOAT3& GetPosition()  const { return mPosition; }
        const DirectX::XMFLOAT4& GetRotation()  const { return mRotation; }
        const DirectX::XMFLOAT3& GetScale()     const { return mScale; }

        DirectX::XMFLOAT2 Get2DPosition() const { return { mPosition.x, mPosition.y }; }
        float Get2DRotation() const;

        // ==============================================================
        //  方向ベクトル
        // ==============================================================

        DirectX::XMVECTOR GetForward() const;
        DirectX::XMVECTOR GetBack()    const;
        DirectX::XMVECTOR GetUp()      const;
        DirectX::XMVECTOR GetDown()    const;
        DirectX::XMVECTOR GetRight()   const;
        DirectX::XMVECTOR GetLeft()    const;

        // ==============================================================
        //  ワールド行列
        // ==============================================================

        const DirectX::XMMATRIX& GetWorldMatrix() const;
        DirectX::XMMATRIX Get2DWorldMatrix() const;

        // ==============================================================
        //  ユーティリティ
        // ==============================================================

        void Reset();
        bool IsDirty() const { return mIsDirty; }

    private:
        void MarkDirty() const { mIsDirty = true; }

        // ---- データ ----
        DirectX::XMFLOAT3 mPosition = { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT4 mRotation = { 0.f, 0.f, 0.f, 1.f };
        DirectX::XMFLOAT3 mScale = { 1.f, 1.f, 1.f };

        // ---- キャッシュ ----
        mutable DirectX::XMMATRIX mCachedMatrix = DirectX::XMMatrixIdentity();
        mutable bool              mIsDirty = true;
    };

} // namespace ecs
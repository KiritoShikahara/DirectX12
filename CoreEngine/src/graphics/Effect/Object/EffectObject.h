#pragma once

#include <Effekseer.h>
#include <DirectXMath.h>

namespace graphics
{
    /// <summary>
    /// Effekseer のハンドルをラップするクラス。
    /// Play / Stop / SetLocation などの操作を EffekseerManager 経由で行う。
    /// EffectComponent のメンバーとして使用する。
    /// </summary>
    class EffectObject
    {
    public:
        EffectObject() = default;
        ~EffectObject() { Stop(); }

        // コピー禁止（ハンドルの二重解放防止）
        EffectObject(const EffectObject&) = delete;
        EffectObject& operator=(const EffectObject&) = delete;

        // ムーブは許可
        EffectObject(EffectObject&& other) noexcept
            : mHandle(other.mHandle), mSpeed(other.mSpeed), mIsAutoDelete(other.mIsAutoDelete)
        {
            other.mHandle = -1;
        }
        EffectObject& operator=(EffectObject&& other) noexcept
        {
            if (this != &other)
            {
                Stop();
                mHandle = other.mHandle;
                mSpeed = other.mSpeed;
                mIsAutoDelete = other.mIsAutoDelete;
                other.mHandle = -1;
            }
            return *this;
        }

        // -----------------------------------------------------------------------
        //  操作
        // -----------------------------------------------------------------------
        void Play(Effekseer::EffectRef effect,
            const DirectX::XMFLOAT3& position,
            bool autoDelete = false);

        void Stop();

        void SetLocation(const DirectX::XMFLOAT3& position);
        void SetRotation(const DirectX::XMFLOAT3& eulerRad);
        void SetScale(const DirectX::XMFLOAT3& scale);
        void SetSpeed(float speed);
        void SetVisible(bool visible);

        /// <summary>
        /// 表示/非表示だけを切り替える(SetVisibleと異なりPauseはしない＝内部シミュレーションは
        /// 進み続ける)。ループ再生の再始動直後のインスタンスを1フレームだけ隠しつつ内部状態を
        /// 進めておきたい場合に使う（EffekseerManager::Update参照）。
        /// </summary>
        void SetRenderingVisible(bool visible);

        // -----------------------------------------------------------------------
        //  状態取得
        // -----------------------------------------------------------------------
        bool IsPlaying()     const;
        bool ShouldDestroy() const { return mIsAutoDelete && !IsPlaying(); }

        Effekseer::Handle GetHandle() const { return mHandle; }

    private:
        Effekseer::Handle mHandle = -1;
        float             mSpeed = 1.f;
        bool              mIsAutoDelete = false;
    };

} // namespace graphics
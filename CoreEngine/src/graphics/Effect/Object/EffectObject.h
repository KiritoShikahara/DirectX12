#pragma once

#include <Effekseer.h>
#include <DirectXMath.h>

namespace graphics
{
    ///<summary>Effekseerのハンドルをラップするクラス。EffectComponentのメンバーとして使う</summary>
    class EffectObject
    {
    public:
        EffectObject() = default;
        ~EffectObject() { Stop(); }

        ///<summary>コピー禁止(ハンドルの二重解放防止)</summary>
        EffectObject(const EffectObject&) = delete;
        EffectObject& operator=(const EffectObject&) = delete;

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

        void Play(Effekseer::EffectRef effect,
            const DirectX::XMFLOAT3& position,
            bool autoDelete = false);

        void Stop();

        void SetLocation(const DirectX::XMFLOAT3& position);
        void SetRotation(const DirectX::XMFLOAT3& eulerRad);
        void SetScale(const DirectX::XMFLOAT3& scale);
        void SetSpeed(float speed);
        void SetVisible(bool visible);

        ///<summary>表示のみ切替(Pauseしない)。ループ再始動直後の1フレーム隠しに使う</summary>
        void SetRenderingVisible(bool visible);

        bool IsPlaying()     const;
        bool ShouldDestroy() const { return mIsAutoDelete && !IsPlaying(); }

        Effekseer::Handle GetHandle() const { return mHandle; }

    private:
        Effekseer::Handle mHandle = -1;
        float             mSpeed = 1.f;
        bool              mIsAutoDelete = false;
    };

} // namespace graphics

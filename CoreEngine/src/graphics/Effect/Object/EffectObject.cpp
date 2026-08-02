#include "pch.h"
#include "EffectObject.h"
#include"../Manager/EffectManager.h"

namespace graphics
{
    void EffectObject::Play(
        Effekseer::EffectRef     effect,
        const DirectX::XMFLOAT3& position,
        bool                     autoDelete)
    {
        Stop();
        mIsAutoDelete = autoDelete;

        auto& mgr = EffekseerManager::Get();
        if (!mgr.IsInitialized() || effect == nullptr) return;

        mHandle = mgr.GetManager()->Play(
            effect, position.x, position.y, position.z);

        if (mHandle >= 0)
            mgr.GetManager()->SetSpeed(mHandle, mSpeed);
    }

    void EffectObject::Stop()
    {
        if (mHandle < 0) return;
        auto& mgr = EffekseerManager::Get();
        if (mgr.IsInitialized())
            mgr.GetManager()->StopEffect(mHandle);
        mHandle = -1;
    }

    void EffectObject::SetLocation(const DirectX::XMFLOAT3& position)
    {
        if (mHandle < 0) return;
        auto& mgr = EffekseerManager::Get();
        if (mgr.IsInitialized())
            mgr.GetManager()->SetLocation(mHandle, position.x, position.y, position.z);
    }

    void EffectObject::SetRotation(const DirectX::XMFLOAT3& eulerRad)
    {
        if (mHandle < 0) return;
        auto& mgr = EffekseerManager::Get();
        if (mgr.IsInitialized())
            mgr.GetManager()->SetRotation(mHandle, eulerRad.x, eulerRad.y, eulerRad.z);
    }

    void EffectObject::SetScale(const DirectX::XMFLOAT3& scale)
    {
        if (mHandle < 0) return;
        auto& mgr = EffekseerManager::Get();
        if (mgr.IsInitialized())
            mgr.GetManager()->SetScale(mHandle, scale.x, scale.y, scale.z);
    }

    void EffectObject::SetSpeed(float speed)
    {
        mSpeed = speed;
        if (mHandle < 0) return;
        auto& mgr = EffekseerManager::Get();
        if (mgr.IsInitialized())
            mgr.GetManager()->SetSpeed(mHandle, mSpeed);
    }

    void EffectObject::SetVisible(bool visible)
    {
        if (mHandle < 0) return;
        auto& mgr = EffekseerManager::Get();
        if (!mgr.IsInitialized()) return;
        mgr.GetManager()->SetShown(mHandle, visible);
        mgr.GetManager()->SetPaused(mHandle, !visible);
    }

    void EffectObject::SetRenderingVisible(bool visible)
    {
        if (mHandle < 0) return;
        auto& mgr = EffekseerManager::Get();
        if (!mgr.IsInitialized()) return;
        mgr.GetManager()->SetShown(mHandle, visible);
    }

    bool EffectObject::IsPlaying() const
    {
        if (mHandle < 0) return false;
        auto& mgr = EffekseerManager::Get();
        if (!mgr.IsInitialized()) return false;
        return mgr.GetManager()->Exists(mHandle);
    }

} // namespace graphics
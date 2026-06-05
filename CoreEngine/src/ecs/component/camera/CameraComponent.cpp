#include"pch.h"
#include "CameraComponent.h"

#include<system/Window/Window.h>
#include<ecs/component/transform/TransformComponent.h>

namespace ecs
{
	void CameraComponent::SetAspectRatioFromWindow(const sys::Window& window)
	{
		const float w = static_cast<float>(window.GetVirtualWidth());
		const float h = static_cast<float>(window.GetVirtualHeight());
		if (h > 0.f) AspectRatio = w / h;
	}

    void CameraComponent::UpdateMatrices(const Transform& transform)
    {
        using namespace DirectX;

        const XMVECTOR eye = XMLoadFloat3(&transform.GetPosition());
        const XMVECTOR forward = transform.GetForward();
        const XMVECTOR up = transform.GetUp();
        const XMVECTOR target = XMVectorAdd(eye, forward);

        const XMMATRIX view = XMMatrixLookAtLH(eye, target, up);
        XMStoreFloat4x4(&ViewMatrix, view);

        const XMMATRIX proj = XMMatrixPerspectiveFovLH(
            XMConvertToRadians(Fov), AspectRatio, Near, Far);
        XMStoreFloat4x4(&ProjectionMatrix, proj);

        XMStoreFloat4x4(&ViewProjectionMatrix, view * proj);
        XMStoreFloat3(&Position, eye);
    }

    DirectX::XMMATRIX CameraComponent::GetViewMatrix() const
    {
        return DirectX::XMLoadFloat4x4(&ViewMatrix);
    }

    DirectX::XMMATRIX CameraComponent::GetProjectionMatrix() const
    {
        return DirectX::XMLoadFloat4x4(&ProjectionMatrix);
    }

    DirectX::XMMATRIX CameraComponent::GetViewProjectionMatrix() const
    {
        return DirectX::XMLoadFloat4x4(&ViewProjectionMatrix);
    }
}


#include "apppch.h"
#include "EnemyHealthBarSystem.h"

#include"EnemyHealthBarTag.h"
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<system/Camera/CameraSystem.h>
#include<system/Window/Window.h>

namespace ecs
{
	void EnemyHealthBarSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto& cameraSys = ::sys::CameraSystem::Get();
		if (!cameraSys.HasMainCamera()) return;

		const auto* camera = registry.try_get<ecs::CameraComponent>(cameraSys.GetMainCameraEntity());
		if (camera == nullptr) return;

		auto& window = ::sys::Window::Get();
		const float screenWidth = static_cast<float>(window.GetVirtualWidth());
		const float screenHeight = static_cast<float>(window.GetVirtualHeight());
		const DirectX::XMMATRIX viewProj = camera->GetViewProjectionMatrix();

		mExpired.clear();

		using namespace DirectX;

		registry.view<EnemyHealthBarTag, Sprite, Transform>().each(
			[&](entt::entity entity, EnemyHealthBarTag& tag, Sprite& sprite, Transform& tr)
			{
				const auto* ownerStatus = registry.valid(tag.Owner)
					? registry.try_get<EnemyStatusComponent>(tag.Owner) : nullptr;
				const auto* ownerTransform = registry.valid(tag.Owner)
					? registry.try_get<Transform>(tag.Owner) : nullptr;

				// オーナーの敵が破棄済み(死亡等)ならバーも道連れに破棄する
				if (ownerStatus == nullptr || ownerTransform == nullptr)
				{
					mExpired.push_back(entity);
					return;
				}

				// ワールド座標(頭上オフセット込み)をクリップ空間へ変換
				XMFLOAT3 worldPos = ownerTransform->GetPosition();
				worldPos.y += tag.HeightOffset;

				const XMVECTOR worldPos4 = XMVectorSetW(XMLoadFloat3(&worldPos), 1.0f);
				const XMVECTOR clip = XMVector4Transform(worldPos4, viewProj);
				const float w = XMVectorGetW(clip);

				// カメラの後方付近はw<=0でNDC変換が破綻するため非表示にする
				if (w <= 0.01f)
				{
					sprite.IsVisible = false;
					return;
				}

				XMFLOAT3 ndc;
				XMStoreFloat3(&ndc, XMVectorScale(clip, 1.0f / w));

				// NDC→スクリーン座標
				tr.Set2DPosition(
					(ndc.x * 0.5f + 0.5f) * screenWidth,
					(1.0f - (ndc.y * 0.5f + 0.5f)) * screenHeight);
				sprite.IsVisible = true;

				if (tag.IsFill)
				{
					const float maxHp = ownerStatus->Current.MaxHp;
					sprite.FillAmount = (maxHp > 0.0f)
						? std::clamp(ownerStatus->CurrentHp / maxHp, 0.0f, 1.0f)
						: 0.0f;
				}
			});

		for (entt::entity entity : mExpired)
		{
			registry.destroy(entity);
		}
	}
}

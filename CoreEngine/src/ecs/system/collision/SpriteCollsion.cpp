#include "pch.h"
#include "SpriteCollsion.h"

#include<graphics/Texture/Texture.h>
#include<ecs/component/sprite/SpriteComponent.h>
#include<ecs/component/transform/TransformComponent.h>
#include<system/Input/InputManager.h>

namespace ecs
{
    bool SpriteCollision::IsPointOver(const Transform& tr, const Sprite& sp, DirectX::XMFLOAT2 point)
    {
		if (!sp.IsVisible || !sp.Texture) return false;

		DirectX::XMFLOAT2 local;
		if (!TryTransformToLocal(tr, sp, point, local)) return false;

		// CalculateShaderData と同じワールド変換規則
		// （Pivot → Scale+Flip → RotationZ → Translation）の逆変換により、
		// ローカル空間は単位クワッド (0,0)〜(1,1) になる。
		return (local.x >= 0.0f && local.x <= 1.0f &&
			local.y >= 0.0f && local.y <= 1.0f);
	}
	bool SpriteCollision::IsMouseOver(const Transform& tr, const Sprite& sp)
	{
		const auto& pos = sys::InputManager::Get().GetMouse()->GetPosition();
		return IsPointOver(tr, sp, pos);
	}

	bool SpriteCollision::TryTransformToLocal(const Transform& tr, const Sprite& sp, DirectX::XMFLOAT2 point, DirectX::XMFLOAT2& outLocal)
	{
		using namespace DirectX;

		// 描画サイズの決定（SpriteRenderer::CalculateShaderData と同じロジック）
		const float baseW = (sp.Size.x > 0.0f) ? sp.Size.x : sp.Texture->GetWidth();
		const float baseH = (sp.Size.y > 0.0f) ? sp.Size.y : sp.Texture->GetHeight();
		const float w = baseW * sp.DrawScale.x;
		const float h = baseH * sp.DrawScale.y;

		// スケールが 0 だと逆行列が作れないため弾く
		if (w == 0.0f || h == 0.0f) return false;

		const XMFLOAT2 pos2D = tr.Get2DPosition();
		const float    rotRad = tr.Get2DRotation();

		// SpriteRenderer::CalculateShaderData と同じ合成順
		//   Pivot（基準点オフセット） → Scale + Flip → Rotation(Z) → Translation
		const XMMATRIX mPivot = XMMatrixTranslation(-sp.Pivot.x, -sp.Pivot.y, 0.0f);
		const XMMATRIX mScale = XMMatrixScaling(w * sp.Flip.x, h * sp.Flip.y, 1.0f);
		const XMMATRIX mRot = XMMatrixRotationZ(rotRad);
		const XMMATRIX mTrans = XMMatrixTranslation(pos2D.x, pos2D.y, 0.0f);
		const XMMATRIX world = mPivot * mScale * mRot * mTrans;

		XMVECTOR det;
		const XMMATRIX invWorld = XMMatrixInverse(&det, world);

		// 行列が特異（逆行列なし）の場合は判定不能
		if (XMVector4Equal(det, XMVectorZero())) return false;

		const XMVECTOR worldPoint = XMVectorSet(point.x, point.y, 0.0f, 1.0f);
		const XMVECTOR localPoint = XMVector3TransformCoord(worldPoint, invWorld);

		outLocal.x = XMVectorGetX(localPoint);
		outLocal.y = XMVectorGetY(localPoint);

		return true;
	}
}


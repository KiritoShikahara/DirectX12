#pragma once

#include<DirectXMath.h>
#include<Utility/Export/Export.h>

namespace ecs
{
	///<summary>
	///現在の移動意図方向を表す汎用コンポーネント。RigidBodyComponent::MoveVelocityはフレーム末にクリアされるため、回転などの継続的な移動方向参照はこちらを使う。プレイヤー・敵共に流用可能
	///</summary>
	struct ENGINE_API MoveDirectionComponent
	{
		///<summary>
		///正規化済みの移動方向。最後に移動していた向きを保持
		///</summary>
		DirectX::XMFLOAT3 Direction = { 0.f, 0.f, 1.f };

		///<summary>
		///今のフレームで移動中かどうか
		///</summary>
		bool IsMoving = false;
	};
}

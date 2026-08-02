#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>

namespace data { struct AreaAttackWeaponData; }

namespace ecs::areaattack
{
	///<summary>
	///1体の敵の座標に氷柱ハザードエンティティを1体生成する。手動・自動どちらのSystemにも属さずここへ切り出す
	///</summary>
	void SpawnHazard(
		entt::registry& registry,
		const DirectX::XMFLOAT3& position,
		float radius,
		float damage,
		const data::AreaAttackWeaponData& masterData);
}

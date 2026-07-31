#pragma once

#include<entt/entt.hpp>
#include<DirectXMath.h>

namespace data { struct AreaAttackWeaponData; }

namespace ecs::areaattack
{
	/// <summary>
	/// 1体の敵の座標に氷柱(ハザード)エンティティを1体生成する。
	/// AreaAttackWeaponSystem(手動発動)とAreaAttackAutoStrikeSystem(自動発動)の
	/// 両方から呼ばれる共通処理のため、どちらのSystemにも属さずここへ切り出す
	/// (Systemクラス同士が直接依存し合うと疎結合の原則に反するため)。
	/// 生成したハザードの持続ダメージ処理はAreaAttackHazardSystemが別途担当する。
	/// </summary>
	void SpawnHazard(
		entt::registry& registry,
		const DirectX::XMFLOAT3& position,
		float radius,
		float damage,
		const data::AreaAttackWeaponData& masterData);
}

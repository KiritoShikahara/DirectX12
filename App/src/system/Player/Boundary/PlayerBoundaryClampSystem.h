#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// プレイヤーがフィールド境界(FieldConstants::kPlayableHalfExtent)の外に出ないよう、
	/// 毎フレーム位置をクランプする最終防衛ライン。
	///
	/// 通常の歩行はCreateFieldBoundaryが生成する物理壁(静的Collider)で防げるが、
	/// Flicker Strikeのワープ(FlickerStrikeWeaponSystem::WarpAndHit)は近くの敵の座標を
	/// 無条件に信用してTransform::SetPositionで瞬間移動するため、壁際の敵(スポーン後は
	/// 再クランプされない)へワープした際に境界の外へ出てしまう経路になっていた。
	/// PostUpdateの最後(PlayerUltimateSystemの後、CameraPlayerFollowSystemの前)に置くことで、
	/// このフレーム中に起きた全てのワープ・移動(Flicker Strikeの通常Update、必殺技の
	/// PostUpdateでのFinishAndExplode)をまとめて捕捉できる。
	/// </summary>
	class PlayerBoundaryClampSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}

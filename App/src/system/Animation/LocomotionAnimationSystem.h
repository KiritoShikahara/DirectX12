#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
	/// <summary>
	/// 移動状態(MoveDirectionComponent::IsMoving)に応じて、FbxAnimComponent の再生クリップを
	/// Idle / Run に切り替えるシステム。プレイヤー・敵を問わず、FbxComponent+FbxAnimComponent+
	/// MoveDirectionComponentを持つ全エンティティが対象(武器の投射物等はFbxAnimComponentを
	/// 持たないため自然に対象外になる)。
	///
	/// ActionAnimLockComponentが付いている間(Attack_A等の単発アクション再生中)はIdle/Runへの
	/// 切り替えを止める。残り時間の減算・解除もこのシステムが担う。
	///
	/// 実際の時間進行・スキニング行列計算は graphics::FbxAnimSystem が行うため、
	/// このシステムは「どのクリップを再生するか(CrossFade)」の決定だけを担う(単一責任)。
	/// 目的クリップが現在と同じ間は何もしないので、毎フレーム呼んでもブレンドは再始動しない。
	/// </summary>
	class LocomotionAnimationSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		// ActionAnimLockComponentの期限切れエンティティの一時バッファ。view走査中の直接removeは
		// イテレータを壊しうるため走査完了後にまとめて外す。毎フレームのvector生成を避けるため
		// メンバで使い回す
		std::vector<entt::entity> mExpiredLocks;
	};
}

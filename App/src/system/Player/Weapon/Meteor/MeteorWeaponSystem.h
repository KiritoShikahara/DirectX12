#pragma once

#include<entt/entt.hpp>
#include<ecs/system/manager/IComponentSystem.h>
#include<DirectXMath.h>
#include<vector>

namespace ecs { struct WeaponComponent; }
namespace data { struct MeteorWeaponData; }

namespace ecs
{
	///<summary>
	///Meteor型武器を処理するシステム。周囲の敵複数体の頭上へ隕石を落とす完全自動の範囲攻撃武器
	///</summary>
	class MeteorWeaponSystem : public ecs::IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///SearchRadius内の敵からランダムに最大MeteorCount体を選び隕石を落とす、不発ならfalseを返す
		///</summary>
		bool Fire(
			entt::registry& registry,
			const ecs::WeaponComponent& weapon,
			const data::MeteorWeaponData& masterData);

		///<summary>
		///1体分の隕石落下、範囲ダメージを与えワンショットエフェクトを再生する
		///</summary>
		void Strike(
			entt::registry& registry,
			const DirectX::XMFLOAT3& position,
			float hitRadius,
			float damage,
			float visualRadius,
			const data::MeteorWeaponData& masterData);

		///<summary>
		///FireのOverlapSphere結果/敵フィルタ結果の一時バッファ、毎回clearして再利用する
		///</summary>
		std::vector<entt::entity> mFound;
		std::vector<entt::entity> mEnemies;

		///<summary>
		///StrikeのOverlapSphere結果の一時バッファ、隕石1発ごとにclearして再利用する
		///</summary>
		std::vector<entt::entity> mOverlapped;
	};
}

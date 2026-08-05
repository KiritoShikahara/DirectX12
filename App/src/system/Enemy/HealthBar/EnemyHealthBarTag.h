#pragma once

#include<entt/entt.hpp>

namespace ecs
{
	///<summary>
	///敵の頭上体力バーを構成するスプライト(背景/前景)に付与するタグ。
	///EnemyHealthBarSystemがOwnerを見てスクリーン座標への追従とFillAmountの更新、
	///Ownerが破棄された(死亡等)ときの自動破棄を行う
	///</summary>
	struct EnemyHealthBarTag
	{
		///<summary>体力バーの持ち主となる敵エンティティ</summary>
		entt::entity Owner = entt::null;

		///<summary>false=背景(常時表示)、true=前景(残量に応じてFillAmountで縮む)</summary>
		bool IsFill = false;

		///<summary>Ownerのワールド原点から見た表示位置の上方向オフセット(ワールド単位)</summary>
		float HeightOffset = 40.0f;
	};
}

#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// 所持武器アイコンバー(GameSceneFactory::CreateWeaponIconBarが生成した10スロット)の
	/// 表示内容を毎フレーム更新するシステム。
	///
	/// スロットごとに以下を行う:
	///   - 対応する武器が所持されていればアイコンテクスチャを反映し表示する(無ければ非表示)
	///   - WeaponCooldownRegistry::TryGetWeaponCooldownで残り/最大クールダウン秒数を取得し、
	///     黒半透明オーバーレイのFillAmount(Radial、時計回りに消える)へ反映する
	///   - 残り秒数をテキストで表示する(クールダウン無し、または発射可能な間は非表示)
	/// </summary>
	class WeaponIconBarSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}

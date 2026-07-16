#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include"PerkSelectComponent.h"
#include"PerkDefinition.h"

namespace ecs
{
	/// <summary>
	/// GameState::PerkSelect 状態の一連の処理を担当する。
	/// - PerkSelect状態に入った最初のフレームでランダムに3択を生成しUIを表示する
	/// - MenuLeft/MenuRightで選択、Selectで確定
	/// - 確定した効果をPlayerStatusComponent/WeaponComponentへ適用する
	/// - UIを破棄し GameStateComponent::PerkSelectDone を立てて GameStateSystem へ戻す
	///
	/// GameStateSystem はステート遷移のみを担当し、パーク固有の知識を持たせない
	/// （疎結合。UI生成・入力・効果適用は全てこのシステムに閉じる）。
	/// </summary>
	class PerkSelectSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		static void EnterPerkSelect(entt::registry& registry, entt::entity controllerEntity);
		static void HandleInput(entt::registry& registry, entt::entity controllerEntity, PerkSelectComponent& select);
		static void ApplyPerk(entt::registry& registry, const PerkDefinition& perk);
		static void ExitPerkSelect(entt::registry& registry, entt::entity controllerEntity);
	};
}

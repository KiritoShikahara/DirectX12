#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include"PerkSelectComponent.h"
#include"PerkDefinition.h"

namespace ecs
{
	///<summary>
	///GameState::PerkSelect状態の処理を担当する。3択の生成・UI表示、選択入力、効果適用、GameStateComponent::PerkSelectDoneを立ててGameStateSystemへ戻すまでを行う
	///</summary>
	class PerkSelectSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///candidatesから指定種別のものを1つランダムに取り出す。取り出した要素は削除し、該当が無ければ-1を返す
		///</summary>
		static int TakeByType(
			std::vector<int>& candidates,
			const std::vector<PerkDefinition>& pool,
			ePerkEffectType type);

		///<summary>
		///candidatesから武器系以外を1つ、PerkData::Weightの重み付きで取り出す。該当が無ければ-1を返す
		///</summary>
		static int TakeWeighted(
			std::vector<int>& candidates,
			const std::vector<PerkDefinition>& pool);

		static void EnterPerkSelect(entt::registry& registry, entt::entity controllerEntity);
		static void HandleInput(entt::registry& registry, entt::entity controllerEntity, PerkSelectComponent& select);
		static void ApplyPerk(entt::registry& registry, const PerkDefinition& perk);
		static void ExitPerkSelect(entt::registry& registry, entt::entity controllerEntity);

		///<summary>
		///選択が確定したプールインデックスの選択回数を+1する
		///</summary>
		static void IncrementPerkPickCount(entt::registry& registry, int poolIndex);
	};
}

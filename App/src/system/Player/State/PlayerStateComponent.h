#pragma once

#include<Utility/Export/Export.h>
#include<unordered_map>
#include<vector>
#include<ranges>

namespace ecs
{
	enum class ePlayerState
	{
		Idle, // 待機
		Move, // 移動
	};

	/// <summary>
	/// 状態遷移判定用のマップ
	/// key= 今の状態 value= 遷移可能な状態のリスト
	/// </summary>
	using PlayerTransitionState = std::unordered_map<ePlayerState, std::vector<ePlayerState>>;

	/// <summary>
	/// 状態遷移リクエスト。優先度順に遷移可能判定
	/// </summary>
	struct PlayerStateRequest
	{
		ePlayerState State;
		int Priority = 0;
	};

	struct ENGINE_API PlayerStateComponent
	{
		// 現在の状態
		ePlayerState CurrentState = ePlayerState::Idle;

		/// <summary>
		/// 状態遷移マップ
		/// </summary>
		PlayerTransitionState StateTransitionMap;

		/// <summary>
		/// 状態遷移リクエストのキャッシュ
		/// </summary>
		std::vector<PlayerStateRequest> Requests;

		bool CanTransition(ePlayerState next)
		{
			// 登録判定
			const auto it = StateTransitionMap.find(CurrentState);
			if (it == StateTransitionMap.end()) return false;

			// 遷移先
			const auto& list = it->second;
			return std::find(list.begin(), list.end(), next) != list.end();
		}

		void AddTransitionMap(ePlayerState from, ePlayerState to)
		{
			auto& list = StateTransitionMap[from];
			if (std::find(list.begin(), list.end(), to) == list.end())
			{
				list.push_back(to);
			}
		}

		/// <summary>
		/// リクエストを追加する
		/// </summary>
		void AddRequest(ePlayerState state, int priority)
		{
			Requests.push_back({ state, priority });
		}

	};

}
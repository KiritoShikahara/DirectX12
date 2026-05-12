#include"pch.h"
#include"EntityManager.h"
#include"EntityTag.h"

namespace ecs
{
	/// <summary>
	/// 初期化
	/// </summary>
	/// <returns>true:成功 false:失敗</returns>
	bool EntityManager::Initialize()
	{
		AllClear();
		DEBUG_LOG(sys::eLogLevel::Log, "EntityManager initialized successfully.");
		return true;
	}

	/// <summary>
	/// 通常のエンティティ作成
	/// </summary>
	/// <returns></returns>
	entt::entity EntityManager::CreateEntity()
	{
		return mRegistry.create();
	}

	/// <summary>
	/// 永続（シーンをまたぐ）の作成
	/// </summary>
	/// <returns></returns>
	entt::entity EntityManager::CreatePersistentEntity()
	{
		auto entity = mRegistry.create();
		mRegistry.emplace<PersistentTag>(entity);
		return entity;
	}

	/// <summary>
	/// エンティティの遅延削除
	/// </summary>
	/// <param name="entity"></param>
	void EntityManager::DestroyDeferred(entt::entity entity)
	{
		// 既にキューに入っていないか確認（重複防止）
		if (mRegistry.valid(entity))
		{
			auto it = std::find(mDestroyQueue.begin(), mDestroyQueue.end(), entity);
			if (it == mDestroyQueue.end())
			{
				mDestroyQueue.push_back(entity);
			}
		}
	}

	/// <summary>
	/// 状態更新（削除など）
	/// </summary>
	void EntityManager::Update()
	{
		if (mDestroyQueue.empty()) return;

		// まとめて削除（validなものだけを対象にする）
		for (auto entity : mDestroyQueue)
		{
			if (mRegistry.valid(entity))
			{
				mRegistry.destroy(entity);
			}
		}
		mDestroyQueue.clear();
	}

	/// <summary>
	/// Localエンティティの削除
	/// </summary>
	void EntityManager::ClearLocalEntities()
	{
		// PersistentTag を持っていないエンティティをすべて抽出して一括破棄
		auto view = mRegistry.view<entt::entity>(entt::exclude<PersistentTag>);
		mRegistry.destroy(view.begin(), view.end());
	}

	/// <summary>
	/// 全ての削除
	/// </summary>
	void EntityManager::AllClear()
	{
		mRegistry.clear();
		mDestroyQueue.clear();
	}

	entt::registry& EntityManager::GetRegistry()
	{
		return mRegistry;
	}






}
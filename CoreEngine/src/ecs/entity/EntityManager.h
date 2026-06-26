#pragma once

#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>
#include<entt/entt.hpp>

#include<vector>

namespace ecs
{

	/// <summary>
	/// エンティティ管理クラス
	/// </summary>
	class EntityManager : public utility::Singleton<EntityManager>
	{
		SINGLETON_CLASS(EntityManager);
	public:
		SINGLETON_ACCESSOR(EntityManager);

		/// <summary>
		/// 初期化
		/// </summary>
		/// <returns>true:成功 false:失敗</returns>
		bool Initialize();

		/// <summary>
		/// 通常のエンティティ作成
		/// </summary>
		/// <returns></returns>
		[[nodiscard]] entt::entity CreateEntity();

		/// <summary>
		/// 永続（シーンをまたぐ）の作成
		/// </summary>
		/// <returns></returns>
		[[nodiscard]] entt::entity CreatePersistentEntity();

		/// <summary>
		/// エンティティの遅延削除
		/// </summary>
		/// <param name="entity"></param>
		void DestroyDeferred(entt::entity entity);

		/// <summary>
		/// 状態更新（削除など）
		/// </summary>
		void Update();

		/// <summary>
		/// Localエンティティの削除
		/// </summary>
		void ClearLocalEntities();

		/// <summary>
		/// 全ての削除
		/// </summary>
		void AllClear();

		template<typename T, typename... Args>
		T& AddComponent(entt::entity entity, Args&&... args) {
			return mRegistry.emplace<T>(entity, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent(entt::entity entity) {
			return mRegistry.get<T>(entity);
		}

		template<typename T>
		bool HasComponent(entt::entity entity) {
			return mRegistry.all_of<T>(entity);
		}

		/// <summary>
		/// Registryへのアクセス
		/// </summary>
		[[nodiscard]] entt::registry& GetRegistry();

	private:
		/// <summary>
		/// 全ての管理のレジストリ
		/// </summary>
		entt::registry mRegistry;
		/// <summary>
		/// 削除予定のエンティティリスト
		/// </summary>
		std::vector<entt::entity> mDestroyQueue;
	};
}

#define ENTITY_MANAGER ::ecs::EntityManager::Get()
#define ENTT_REGISTRY ENTITY_MANAGER.GetRegistry();

#define CREATE_ENTITY ENTITY_MANAGER.CreateEntity();
#define CREATE_LOCAL_ENTITY ENTITY_MANAGER.CreatePersistentEntity();
#define ADD_COMPONENT(CLASS) ENTITY_MANAGER.AddComponent<CLASS>();


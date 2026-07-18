#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<Utility/Thread/ThreadPool.h>
#include<DirectXMath.h>
#include<vector>
#include<functional>

namespace ecs
{
	/// <summary>
	/// 敵をプレイヤーへ追従移動させるシステム。
	/// 各敵の計算は他の敵のコンポーネントに一切触れない(自分自身のTransform/RigidBody/
	/// MoveDirectionComponentのみを読み書きし、エンティティの生成・破棄も行わない)ため、
	/// 敵数がkParallelThreshold以上になった場合は専用のThreadPoolで区間分割し並列処理する。
	/// (entt::registryはこの並列区間中に構造変更が起きない限り、異なるエンティティに対する
	/// registry.get<T>()の並行呼び出しは安全なため)。
	/// </summary>
	class EnemyChaseSystem: public IUserSystem
	{
	public:
		EnemyChaseSystem();
		~EnemyChaseSystem() override;

		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>1体分の追従移動処理(逐次実行・並列実行の両方から共通で呼ばれる)</summary>
		void ProcessOne(entt::registry& registry, entt::entity entity, const DirectX::XMFLOAT3& playerPos) const;

		// Update()の一時バッファ(対象敵一覧)。毎回clear()して再利用する
		// (毎フレームのvector生成禁止のため)
		std::vector<entt::entity> mEnemies;

		// 追従移動計算のバッチ並列化専用ワーカープール(このSystemが所有・管理する)
		utility::ThreadPool mThreadPool;

		// Dispatch()へ渡すタスク配列。毎フレーム生成すると(std::function自体のヒープ確保も
		// 含めて)無視できないアロケーションコストになるため、ワーカー数分だけ構築時に確保し、
		// 各要素へ新しいラムダを代入するだけで使い回す(vector自体の再確保は発生しない)
		std::vector<std::function<void()>> mChunkTasks;

		// 敵数がこれ未満の場合はスレッド起床コストの方が大きいため逐次実行する
		static constexpr size_t kParallelThreshold = 64;
	};
}

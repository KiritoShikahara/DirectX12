#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<Utility/Thread/ThreadPool.h>
#include<DirectXMath.h>
#include<vector>
#include<functional>

namespace ecs
{
	///<summary>
	///敵をプレイヤーへ追従移動させるシステム。各敵の計算は自身のコンポーネントのみを読み書きするため、敵数がkParallelThreshold以上ならThreadPoolで並列処理する
	///</summary>
	class EnemyChaseSystem: public IUserSystem
	{
	public:
		EnemyChaseSystem();
		~EnemyChaseSystem() override;

		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///1体分の追従移動処理。逐次実行・並列実行の両方から共通で呼ばれる
		///</summary>
		void ProcessOne(entt::registry& registry, entt::entity entity, const DirectX::XMFLOAT3& playerPos) const;

		///<summary>
		///Updateの一時バッファ、対象敵一覧。毎回clearして再利用し毎フレームのvector生成を避ける
		///</summary>
		std::vector<entt::entity> mEnemies;

		///<summary>
		///追従移動計算のバッチ並列化専用ワーカープール。このSystemが所有・管理する
		///</summary>
		utility::ThreadPool mThreadPool;

		///<summary>
		///Dispatchへ渡すタスク配列。毎フレーム生成すると無視できないアロケーションコストになるためワーカー数分だけ構築時に確保し使い回す
		///</summary>
		std::vector<std::function<void()>> mChunkTasks;

		///<summary>
		///敵数がこれ未満の場合は逐次実行する。実測でThreadPoolの起床コストが支配的になり並列化が逆効果だったため、実際の敵数上限を大きく超える値にして並列パスを事実上無効化している
		///</summary>
		static constexpr size_t kParallelThreshold = 2000;
	};
}

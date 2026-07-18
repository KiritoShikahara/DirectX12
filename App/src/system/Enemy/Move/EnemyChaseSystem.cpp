#include "apppch.h"
#include "EnemyChaseSystem.h"

#include<system/MoveDirection/MoveDirectionComponent.h>
#include<Tag/EntityTag.h>
#include"EnemyChaseComponent.h"
#include<system/Enemy/Knockback/EnemyKnockbackComponent.h>

using namespace DirectX;

namespace ecs
{
	EnemyChaseSystem::EnemyChaseSystem()
	{
		// 0指定でハードウェアコア数-1のワーカーを起動する(ThreadPool::Initialize参照)
		mThreadPool.Initialize(0);

		// mChunkTasksはワーカー数分だけ一度確保しておき、以後はUpdate()で再利用する
		// (要素数はワーカー数を超えないため、これ以上の再確保は発生しない)
		mChunkTasks.resize(mThreadPool.WorkerCount());
	}

	EnemyChaseSystem::~EnemyChaseSystem()
	{
		mThreadPool.Finalize();
	}

	void EnemyChaseSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto playerView = registry.view<PlayerTag, Transform>();
		if (playerView.size_hint() == 0)
		{
			// プレイヤー不在なら何もしない
			return;
		}

		const entt::entity playerEntity = *playerView.begin();
		const XMFLOAT3 playerPos = registry.get<Transform>(playerEntity).GetPosition();

		// 毎フレームのvector生成を避けるため、メンバ変数(mEnemies)を使い回す
		mEnemies.clear();
		auto view = registry.view<EnemyTag, EnemyChaseComponent, Transform, RigidBodyComponent, MoveDirectionComponent>();
		mEnemies.reserve(view.size_hint());
		for (entt::entity entity : view)
		{
			mEnemies.push_back(entity);
		}

		if (mEnemies.empty()) return;

		// 敵数が少ない間はスレッド起床コストの方が大きいため逐次実行する
		if (mEnemies.size() < kParallelThreshold)
		{
			for (entt::entity entity : mEnemies)
			{
				ProcessOne(registry, entity, playerPos);
			}
			return;
		}

		// 敵数が多い場合はmEnemiesを互いに素な連続区間へ分割し、ワーカースレッドへ割り当てる。
		// 各区間は他の区間のエンティティに一切触れないため、この並列区間中に構造変更
		// (エンティティ生成・破棄やコンポーネント追加・削除)さえ起きなければデータ競合は
		// 起きない(このSystem自体は構造変更を一切行わない)。
		const size_t workerCount = mThreadPool.WorkerCount();
		const size_t chunkCount = std::min(workerCount, mEnemies.size());

		// mChunkTasks(構築時にワーカー数分だけ確保済み)の先頭chunkCount要素だけを使う。
		// 既存のstd::function要素へラムダを代入するだけなので、vector自体の再確保は起きない
		const size_t baseSize = mEnemies.size() / chunkCount;
		const size_t remainder = mEnemies.size() % chunkCount;

		size_t offset = 0;
		for (size_t i = 0; i < chunkCount; ++i)
		{
			const size_t chunkSize = baseSize + (i < remainder ? 1 : 0);
			const size_t begin = offset;
			const size_t end = offset + chunkSize;
			offset = end;

			mChunkTasks[i] = [this, &registry, begin, end, playerPos]()
			{
				for (size_t j = begin; j < end; ++j)
				{
					ProcessOne(registry, mEnemies[j], playerPos);
				}
			};
		}

		mThreadPool.Dispatch(mChunkTasks.data(), chunkCount);
		mThreadPool.WaitAll();
	}

	/// <summary>1体分の追従移動処理(逐次実行・並列実行の両方から共通で呼ばれる)</summary>
	void EnemyChaseSystem::ProcessOne(entt::registry& registry, entt::entity entity, const XMFLOAT3& playerPos) const
	{
		// ノックバック中はEnemyKnockbackSystemがMoveVelocityを制御するため、
		// 通常の追従移動を上書きしないようここで完全にスキップする
		if (registry.all_of<EnemyKnockbackComponent>(entity)) return;

		auto& chase = registry.get<EnemyChaseComponent>(entity);
		auto& transform = registry.get<Transform>(entity);
		auto& rigidBody = registry.get<RigidBodyComponent>(entity);
		auto& moveDir = registry.get<MoveDirectionComponent>(entity);

		constexpr float kEpsilon = 1e-4f;

		const XMFLOAT3 pos = transform.GetPosition();
		const XMVECTOR vPlayer = XMLoadFloat3(&playerPos);

		// プレイヤーへのベクトル（水平面のみ：Y を無視）
		XMVECTOR toPlayer = XMVectorSubtract(vPlayer, XMLoadFloat3(&pos));
		toPlayer = XMVectorSetY(toPlayer, 0.0f);

		const float dist = XMVectorGetX(XMVector3Length(toPlayer));

		// ほぼ同一座標なら停止（正規化不能）
		if (dist <= kEpsilon)
		{
			moveDir.IsMoving = false;
			// Y方向の残留速度(必殺技中に上昇するプレイヤーと接触して押し上げられた場合等)を
			// 毎フレーム明示的に0へリセットする(GravityFactor=0のため自然には落ちてこない)
			rigidBody.MoveVelocity = { 0.0f, 0.0f, 0.0f };
			rigidBody.HasMoveRequest = true;
			return;
		}

		const XMVECTOR dir = XMVector3Normalize(toPlayer);

		// 向きは間合い内でも更新し続ける（プレイヤーを向く）
		XMStoreFloat3(&moveDir.Direction, dir);

		// 停止間合いより遠いときだけ移動速度を積む（dirのYは常に0のためvelocity.yも常に0）
		if (dist > chase.StopDistance)
		{
			XMFLOAT3 velocity;
			XMStoreFloat3(&velocity, XMVectorScale(dir, chase.MoveSpeed));

			rigidBody.MoveVelocity = velocity;
			rigidBody.HasMoveRequest = true;
			moveDir.IsMoving = true;
		}
		else
		{
			// 間合い内：水平移動はしないが、Y方向の残留速度(必殺技中に上昇するプレイヤーと
			// 接触して押し上げられた場合等)は毎フレーム明示的に0へリセットする
			rigidBody.MoveVelocity = { 0.0f, 0.0f, 0.0f };
			rigidBody.HasMoveRequest = true;
			moveDir.IsMoving = false;
		}
	}
}

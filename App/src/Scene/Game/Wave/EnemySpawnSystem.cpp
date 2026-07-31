#include "apppch.h"
#include "EnemySpawnSystem.h"

#include"WaveComponent.h"
#include<Scene/Game/State/GameState.h>
#include<Scene/Game/Factory/GameSceneFactory.h>
#include<Scene/Game/Factory/FieldConstants.h>
#include<Scene/Game/Debug/GameDebugSettings.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Tag/EntityTag.h>
#include<system/Camera/CameraSystem.h>
#include<system/Window/Window.h>
#include<Data/Enemy/EnemyData.h>

#include<random>
#include<algorithm>

using namespace DirectX;

namespace ecs
{
	namespace
	{
		// 敵の湧き出し高さ(地面のY座標。GameSceneFactory::CreatePlayerの初期Y座標(0.1)と合わせる)。
		// プレイヤーの現在のY座標(playerPos.y)を使うと、必殺技での上昇中に敵がプレイヤーと
		// 同じ高さ(=空中)で湧いてしまい、あたかも空まで追いかけてきたように見えるバグになるため、
		// 地面は常に平面である前提でこの固定値を使う
		constexpr float kSpawnGroundY = 0.1f;

		// プロセス全体で1つの乱数エンジンを使い回す（毎フレーム再生成しない）
		std::mt19937& GetRandomEngine()
		{
			static std::mt19937 engine = ::debug::GameDebugSettings::Get().MakeRandomEngine();
			return engine;
		}

		/// <summary>data::EnemyDataに登録されている敵の種類から一様ランダムに1つ選ぶ
		/// （1種類も登録されていない場合はId=0を返す）</summary>
		int PickRandomEnemyId()
		{
			const auto& enemies = DATA_MGR(data::EnemyData).GetAll();
			if (enemies.empty()) return 0;

			std::uniform_int_distribution<size_t> dist(0, enemies.size() - 1);
			return enemies[dist(GetRandomEngine())].Id;
		}

		/// <summary>
		/// 経過時間から「stepInterval秒ごとにgrowthPerStep分だけ段階的に増加する」倍率を求める共通ヘルパー。
		/// 滑らかな連続成長ではなく階段状にすることで、経過時間に対する体感のメリハリを付ける。
		/// 敵ステータス成長(ComputeWaveModifier)とスポーン数成長(EnemySpawnSystem::Update)の両方で使う。
		/// </summary>
		float ComputeStepGrowth(float elapsedTime, float stepInterval, float growthPerStep)
		{
			const float safeStepInterval = std::max(stepInterval, 1.0f); // 0除算防止
			const float stepCount = std::floor(elapsedTime / safeStepInterval);
			return 1.0f + growthPerStep * stepCount;
		}
	}

	void EnemySpawnSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto stateView = registry.view<::ecs::GameStateComponent, ::ecs::WaveComponent>();
		if (stateView.begin() == stateView.end()) return;

		const entt::entity controllerEntity = *stateView.begin();
		auto& gameState = registry.get<::ecs::GameStateComponent>(controllerEntity);
		if (gameState.GameState != ::sys::eGameState::InGame) return;

		auto& wave = registry.get<::ecs::WaveComponent>(controllerEntity);

		auto playerView = registry.view<::ecs::PlayerTag, ::ecs::Transform>();
		if (playerView.begin() == playerView.end()) return;
		const XMFLOAT3 playerPos = registry.get<::ecs::Transform>(*playerView.begin()).GetPosition();

		wave.ElapsedTime += deltaTime;

		// クリア判定（これ以降のスポーン処理は行わない）
		if (wave.ElapsedTime >= wave.ClearTime)
		{
			gameState.GameClearRequested = true;
			return;
		}

		const ecs::EnemyWaveModifier waveModifier =
			ComputeWaveModifier(wave.ElapsedTime, wave.StatGrowthStepInterval, wave.StatGrowthPerStep);

		// 通常の敵の継続スポーン（1回のタイミングでSpawnCountPerTick体まとめて湧かせる。
		// 重なって湧かないよう、1体ごとに独立してランダムな位置を求める。
		// 敵の種類もdata::EnemyDataに登録されている中からランダムに選ぶ）
		// MaxAliveEnemyが有効(1以上)な場合のみ、生存数が上限に達している間は
		// スポーンを間引く。0以下は「上限なし」を意味する(ecs::WaveComponent参照)
		wave.SpawnTimer -= deltaTime;
		if (wave.SpawnTimer <= 0.0f)
		{
			// 終盤ほど大量の敵が出現するよう、1回のスポーン数もSpawnCountGrowthStepInterval秒ごとに
			// SpawnCountGrowthPerStep分だけ階段状に増やす(敵ステータス成長と同じ式を使い回す)
			const float spawnCountGrowth = ComputeStepGrowth(
				wave.ElapsedTime, wave.SpawnCountGrowthStepInterval, wave.SpawnCountGrowthPerStep);
			int spawnCount = std::max(1, static_cast<int>(std::round(wave.SpawnCountPerTick * spawnCountGrowth)));
			if (wave.MaxAliveEnemy > 0)
			{
				const int aliveCount = static_cast<int>(registry.view<::ecs::EnemyTag>().size());
				const int spawnCapacity = std::max(0, wave.MaxAliveEnemy - aliveCount);
				spawnCount = std::min(spawnCount, spawnCapacity);
			}

			for (int i = 0; i < spawnCount; ++i)
			{
				const XMFLOAT3 spawnPos = ComputeSpawnPosition(
					registry, playerPos, wave.SpawnMarginMin, wave.SpawnMarginMax);
				::ecs::GameSceneFactory::CreateEnemy(spawnPos, waveModifier, ecs::eBossTier::None, PickRandomEnemyId());
			}
			wave.SpawnTimer = wave.SpawnInterval;
		}

		// ボース出現（通常の敵よりさらに奥から出す。種類は常にId=0の強化版、階級の倍率はdata::BossData）。
		// 小ボースは周期的に繰り返し、中ボース・最強ボースはそれぞれ1回だけ出現する。
		if (wave.ElapsedTime >= wave.NextMiniBossSpawnTime)
		{
			const XMFLOAT3 spawnPos = ComputeSpawnPosition(
				registry, playerPos, wave.SpawnMarginMax + 5.0f, wave.SpawnMarginMax + 15.0f);
			::ecs::GameSceneFactory::CreateEnemy(spawnPos, waveModifier, ecs::eBossTier::Mini, 0);
			wave.NextMiniBossSpawnTime += std::max(wave.MiniBossInterval, 1.0f); // 0除算/連続スポーン防止
		}

		if (!wave.MidBossSpawned && wave.ElapsedTime >= wave.MidBossSpawnTime)
		{
			const XMFLOAT3 spawnPos = ComputeSpawnPosition(
				registry, playerPos, wave.SpawnMarginMax + 5.0f, wave.SpawnMarginMax + 15.0f);
			::ecs::GameSceneFactory::CreateEnemy(spawnPos, waveModifier, ecs::eBossTier::Mid, 0);
			wave.MidBossSpawned = true;
		}

		if (!wave.FinalBossSpawned && wave.ElapsedTime >= wave.FinalBossSpawnTime)
		{
			const XMFLOAT3 spawnPos = ComputeSpawnPosition(
				registry, playerPos, wave.SpawnMarginMax + 5.0f, wave.SpawnMarginMax + 15.0f);
			::ecs::GameSceneFactory::CreateEnemy(spawnPos, waveModifier, ecs::eBossTier::Final, 0);
			wave.FinalBossSpawned = true;
		}
	}

	/// <summary>画面外(画面に映っている範囲の半径 + マージン)のリング上にランダムなスポーン位置を求める</summary>
	DirectX::XMFLOAT3 EnemySpawnSystem::ComputeSpawnPosition(
		entt::registry& registry,
		const DirectX::XMFLOAT3& playerPos,
		float marginMin, float marginMax)
	{
		const float visibleRadius = ComputeVisibleRadius(registry, playerPos);

		std::uniform_real_distribution<float> angleDist(0.0f, XM_2PI);
		std::uniform_real_distribution<float> marginDist(marginMin, marginMax);

		const float angle = angleDist(GetRandomEngine());
		const float radius = visibleRadius + marginDist(GetRandomEngine());

		float x = playerPos.x + std::cos(angle) * radius;
		float z = playerPos.z + std::sin(angle) * radius;

		// プレイヤー相対の計算だけだと、プレイヤーがフィールド境界(見えない壁、
		// FieldConstants::kPlayableHalfExtent)付近にいる場合、外側方向への抽選で
		// 壁の外にスポーンしてしまい、その敵が壁に阻まれて二度とフィールド内に
		// 入れなくなる(=進行不能)不具合になっていた。壁の内側へ確実に収まるよう、
		// 少し余裕を持たせてクランプする。
		constexpr float kSpawnBoundaryMargin = 50.0f;
		const float limit = FieldConstants::kPlayableHalfExtent - kSpawnBoundaryMargin;
		x = std::clamp(x, -limit, limit);
		z = std::clamp(z, -limit, limit);

		return { x, kSpawnGroundY, z };
	}

	/// <summary>
	/// 現在のカメラ設定で、プレイヤーの足元平面上に画面(四隅)が投影される範囲の半径を求める。
	/// カメラは常にプレイヤーへ一定オフセットで追従するため、この値は実質プレイ中一定になる。
	/// </summary>
	float EnemySpawnSystem::ComputeVisibleRadius(entt::registry& registry, const DirectX::XMFLOAT3& playerPos)
	{
		constexpr float kFallbackRadius = 30.0f;

		auto& cameraSys = ::sys::CameraSystem::Get();
		if (!cameraSys.HasMainCamera())
		{
			return kFallbackRadius;
		}

		auto& window = ::sys::Window::Get();
		const float w = static_cast<float>(window.GetVirtualWidth());
		const float h = static_cast<float>(window.GetVirtualHeight());

		const XMFLOAT2 corners[4] = { { 0.0f, 0.0f }, { w, 0.0f }, { 0.0f, h }, { w, h } };

		float maxDistSq = 0.0f;
		bool anyHit = false;
		for (const auto& corner : corners)
		{
			XMFLOAT3 worldPos;
			if (!cameraSys.ScreenPointToWorldOnPlaneY(registry, corner, playerPos.y, worldPos))
			{
				continue;
			}

			anyHit = true;
			const float dx = worldPos.x - playerPos.x;
			const float dz = worldPos.z - playerPos.z;
			maxDistSq = std::max(maxDistSq, dx * dx + dz * dz);
		}

		return anyHit ? std::sqrt(maxDistSq) : kFallbackRadius;
	}

	/// <summary>
	/// 経過時間から現在の敵ステータス成長倍率を求める。滑らかな連続成長ではなく、
	/// stepInterval(秒)ごとにgrowthPerStep分だけ段階的に強くなる階段状にする
	/// (例: 2分ごとに+40%なら、0-2分=等倍、2-4分=1.4倍、4-6分=1.8倍…と2分単位で跳ね上がる)。
	/// </summary>
	ecs::EnemyWaveModifier EnemySpawnSystem::ComputeWaveModifier(float elapsedTime, float stepInterval, float growthPerStep)
	{
		ecs::EnemyWaveModifier modifier;

		const float growth = ComputeStepGrowth(elapsedTime, stepInterval, growthPerStep);

		modifier.MulMaxHp = growth;
		modifier.MulAtkPower = growth;

		// 移動速度は意図的にスケールしない（敵がプレイヤーより速くなり続けるのを防ぐため）
		modifier.MulMoveSpeed = 1.0f;

		return modifier;
	}
}

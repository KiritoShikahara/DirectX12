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
		constexpr float kSpawnGroundY = 0.1f;

		std::mt19937& GetRandomEngine()
		{
			// プロセス全体で使い回す
			static std::mt19937 engine = ::debug::GameDebugSettings::Get().MakeRandomEngine();
			return engine;
		}

		int PickRandomEnemyId()
		{
			const auto& enemies = DATA_MGR(data::EnemyData).GetAll();
			if (enemies.empty()) return 0;

			std::uniform_int_distribution<size_t> dist(0, enemies.size() - 1);
			return enemies[dist(GetRandomEngine())].Id;
		}

		float ComputeStepGrowth(float elapsedTime, float stepInterval, float growthPerStep)
		{
			// 階段状に成長させる共通ヘルパー。滑らかな連続成長にしないことで経過時間に対する体感のメリハリを付ける
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

		// クリア判定。これ以降のスポーン処理は行わない
		if (wave.ElapsedTime >= wave.ClearTime)
		{
			gameState.GameClearRequested = true;
			return;
		}

		const ecs::EnemyWaveModifier waveModifier =
			ComputeWaveModifier(wave.ElapsedTime, wave.StatGrowthStepInterval, wave.StatGrowthPerStep);

		// 通常の敵の継続スポーン。1回でSpawnCountPerTick体を独立した位置にまとめて湧かせ、種類はEnemyDataからランダムに選ぶ。MaxAliveEnemyが有効なら上限に達している間は間引く
		wave.SpawnTimer -= deltaTime;
		if (wave.SpawnTimer <= 0.0f)
		{
			// 終盤ほど多く出現するようSpawnCountGrowthStepInterval秒ごとにSpawnCountGrowthPerStep分だけ階段状にスポーン数を増やす
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

		// ボス出現は通常の敵よりさらに奥から出す。種類は常にId=0の強化版で階級の倍率はdata::BossData。
		// 小ボス・中ボスは周期的、大ボスは1回だけ。いずれも通常敵の経過時間成長(waveModifier)とは
		// 別軸の強さで管理し、時間経過バランスの調整がボスの強さに意図せず影響しないようにする
		if (wave.ElapsedTime >= wave.NextMiniBossSpawnTime)
		{
			const XMFLOAT3 spawnPos = ComputeSpawnPosition(
				registry, playerPos, wave.SpawnMarginMax + 5.0f, wave.SpawnMarginMax + 15.0f);

			// 小ボスは出現するたびに直前の小ボスからBossPowerGrowthPerSpawn倍ずつ強くなる。
			// 出現回数だけに依存する指数成長にすることで「Nボス目は必ず(N-1)ボス目の何倍」という
			// 強さの比率を、経過時間ベースのバランス調整と切り離して管理できる
			const float miniPower = std::pow(
				wave.BossPowerGrowthPerSpawn, static_cast<float>(wave.MiniBossSpawnCount));
			ecs::EnemyWaveModifier miniModifier;
			miniModifier.MulMaxHp = miniPower;
			miniModifier.MulAtkPower = miniPower;
			miniModifier.MulMoveSpeed = 1.0f; // 通常敵と同様、移動速度はプレイヤーより速くなり続けないよう固定

			::ecs::GameSceneFactory::CreateEnemy(spawnPos, miniModifier, ecs::eBossTier::Mini, 0);
			wave.MiniBossSpawnCount += 1;
			wave.NextMiniBossSpawnTime += std::max(wave.MiniBossInterval, 1.0f); // 0除算/連続スポーン防止
		}

		if (wave.ElapsedTime >= wave.NextMidBossSpawnTime)
		{
			const XMFLOAT3 spawnPos = ComputeSpawnPosition(
				registry, playerPos, wave.SpawnMarginMax + 5.0f, wave.SpawnMarginMax + 15.0f);

			// 中ボスの強さは「直近に出現した小ボス」のMidBossPowerMultiplier倍に追従させる。
			// 小ボスは出現ごとに強くなり続けるため、こうしないと終盤の中ボスが小ボスに埋もれてしまう
			const int latestMiniIndex = std::max(0, wave.MiniBossSpawnCount - 1);
			const float latestMiniPower = std::pow(
				wave.BossPowerGrowthPerSpawn, static_cast<float>(latestMiniIndex));
			const float midPower = wave.MidBossPowerMultiplier * latestMiniPower;

			ecs::EnemyWaveModifier midModifier;
			midModifier.MulMaxHp = midPower;
			midModifier.MulAtkPower = midPower;
			midModifier.MulMoveSpeed = 1.0f;

			::ecs::GameSceneFactory::CreateEnemy(spawnPos, midModifier, ecs::eBossTier::Mid, 0);
			wave.NextMidBossSpawnTime += std::max(wave.MidBossInterval, 1.0f); // 0除算/連続スポーン防止
		}

		if (!wave.FinalBossSpawned && wave.ElapsedTime >= wave.FinalBossSpawnTime)
		{
			const XMFLOAT3 spawnPos = ComputeSpawnPosition(
				registry, playerPos, wave.SpawnMarginMax + 5.0f, wave.SpawnMarginMax + 15.0f);

			// 大ボスは経過時間・小ボス成長と無関係の絶対値。強さはdata::BossDataの倍率だけで決まる
			const ecs::EnemyWaveModifier finalModifier;
			::ecs::GameSceneFactory::CreateEnemy(spawnPos, finalModifier, ecs::eBossTier::Final, 0);
			wave.FinalBossSpawned = true;
		}
	}

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

		// プレイヤーがフィールド境界付近にいると壁の外にスポーンし進行不能になるため、壁の内側へ収まるようクランプする
		constexpr float kSpawnBoundaryMargin = 50.0f;
		const float limit = FieldConstants::kPlayableHalfExtent - kSpawnBoundaryMargin;
		x = std::clamp(x, -limit, limit);
		z = std::clamp(z, -limit, limit);

		return { x, kSpawnGroundY, z };
	}

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

	ecs::EnemyWaveModifier EnemySpawnSystem::ComputeWaveModifier(float elapsedTime, float stepInterval, float growthPerStep)
	{
		ecs::EnemyWaveModifier modifier;

		const float growth = ComputeStepGrowth(elapsedTime, stepInterval, growthPerStep);

		modifier.MulMaxHp = growth;
		modifier.MulAtkPower = growth;

		// 移動速度は意図的にスケールしない。敵がプレイヤーより速くなり続けるのを防ぐため
		modifier.MulMoveSpeed = 1.0f;

		return modifier;
	}
}

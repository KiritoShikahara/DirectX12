#include "apppch.h"
#include "EnemySpawnSystem.h"

#include"WaveComponent.h"
#include<Scene/Game/State/GameState.h>
#include<Scene/Game/Factory/GameSceneFactory.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Tag/EntityTag.h>
#include<system/Camera/CameraSystem.h>
#include<system/Window/Window.h>

#include<random>
#include<algorithm>

using namespace DirectX;

namespace ecs
{
	namespace
	{
		// プロセス全体で1つの乱数エンジンを使い回す（毎フレーム再生成しない）
		std::mt19937& GetRandomEngine()
		{
			static std::mt19937 engine{ std::random_device{}() };
			return engine;
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
			ComputeWaveModifier(wave.ElapsedTime, wave.StatGrowthPerSecond);

		// 通常の敵の継続スポーン
		wave.SpawnTimer -= deltaTime;
		if (wave.SpawnTimer <= 0.0f)
		{
			const XMFLOAT3 spawnPos = ComputeSpawnPosition(
				registry, playerPos, wave.SpawnMarginMin, wave.SpawnMarginMax);
			::ecs::GameSceneFactory::CreateEnemy(spawnPos, waveModifier, false);
			wave.SpawnTimer = wave.SpawnInterval;
		}

		// ボース出現（1回だけ、通常の敵よりさらに奥から出す）
		if (!wave.BossSpawned && wave.ElapsedTime >= wave.BossSpawnTime)
		{
			const XMFLOAT3 spawnPos = ComputeSpawnPosition(
				registry, playerPos, wave.SpawnMarginMax + 5.0f, wave.SpawnMarginMax + 15.0f);
			::ecs::GameSceneFactory::CreateEnemy(spawnPos, waveModifier, true);
			wave.BossSpawned = true;
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

		return
		{
			playerPos.x + std::cos(angle) * radius,
			playerPos.y,
			playerPos.z + std::sin(angle) * radius,
		};
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

	/// <summary>経過時間から現在の敵ステータス成長倍率を求める</summary>
	ecs::EnemyWaveModifier EnemySpawnSystem::ComputeWaveModifier(float elapsedTime, float growthPerSecond)
	{
		ecs::EnemyWaveModifier modifier;

		const float growth = 1.0f + growthPerSecond * elapsedTime;
		modifier.MulMaxHp = growth;
		modifier.MulAtkPower = growth;

		// 移動速度は意図的にスケールしない（敵がプレイヤーより速くなり続けるのを防ぐため）
		modifier.MulMoveSpeed = 1.0f;

		return modifier;
	}
}

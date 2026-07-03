#include "apppch.h"
#include "GameSceneFactory.h"

#include<system/GameStateController/GameStateComponent.h>
#include<system/CameraFollow/CameraFollowOffsetComponent.h>
#include<Tag/EntityTag.h>

namespace ecs
{
	void GameSceneFactory::CreateBGM()
	{
		// 仮の音楽
		PLAY_BGM("Assets/Sound/BGM/BGM_Title.aud", true, 0.7);
	}

	void GameSceneFactory::CreateStateObject()
	{
		auto& manager = ENTITY_MANAGER;
		auto entity = manager.CreateEntity();

		// 状態
		auto& state = manager.AddComponent<::ecs::GameStateComponent>(entity);
	}

	void GameSceneFactory::CreateGround()
	{
		auto& manager = ENTITY_MANAGER;

		auto res = ::graphics::FbxResourceManager::Get().Load("Assets/Fbx/Field/Field.fbx.bin");
		float scale = 20.0f;

		auto entity = manager.CreateEntity();
		auto& tr = manager.AddComponent<::ecs::Transform>(entity);
		tr.SetScale(scale);

		auto& fbx = manager.AddComponent<::ecs::FbxComponent>(entity);
		fbx.Resource = res;
	}

	void GameSceneFactory::CreatePlayer(const CreatePlayerContext& Context)
	{
		// 管理
		auto& manager = ENTITY_MANAGER;
		auto& registry = ENTT_REGISTRY;
		auto player_res = ::graphics::FbxResourceManager::Get().Load("Assets/Fbx/Faul/Faul.fbx.bin");

		// プレイヤーの生成
		auto p_scale = 0.2f;
		auto player = manager.CreateEntity();
		auto& tr = manager.AddComponent<ecs::Transform>(player);
		tr.SetScale(p_scale);
		tr.SetPosition(0, 0.1, 0);

		auto& fbx = manager.AddComponent<ecs::FbxComponent>(player);
		fbx.Resource = player_res;

		manager.AddComponent<ecs::ColliderComponent>(player, ecs::ColliderComponent::MakeBox({ 1,3,1 }));
		manager.AddComponent<ecs::RigidBodyComponent>(player, ecs::RigidBodyComponent::MakeKinematic());

		registry.emplace<::ecs::PlayerTag>(player);

		// 初期武器の生成
		auto weapon = manager.CreateEntity();
	}

	void GameSceneFactory::CreateCamera()
	{
		auto& manager = ENTITY_MANAGER;
		auto entity = manager.CreateEntity();

		auto& tr = manager.AddComponent<::ecs::Transform>(entity);

		auto& cam = manager.AddComponent<::ecs::CameraComponent>(entity);
		cam.IsMainCamera = true;
		cam.Fov = 120.0f;
		cam.Near = 0.1f;
		cam.Far = 1000.0f;
		cam.SetAspectRatioFromWindow(sys::Window::Get());

		auto& follow = manager.AddComponent<::ecs::CameraFollowOffsetComponent>(entity);
		follow.Offset = { 0.f, 72.f, -38.f };
		follow.LookAtOffset = { 0.f, -24.f, 0.f };

	}

	void GameSceneFactory::CreateDirLight()
	{
		auto& manager = ENTITY_MANAGER;
		entt::entity entity = manager.CreateEntity();

		manager.AddComponent<::ecs::Transform>(entity);

		auto& light = manager.AddComponent<ecs::DirectionalLightComponent>(entity);
		light.Direction = { 0.3f, -1.0f, 0.5f };
		light.Color = { 1.0f,  1.0f, 1.0f };
		light.Intensity = 7.5f;
		light.IsActive = true;
		light.CastShadow = true;
		light.ShadowRange = 50.0f;
		light.ShadowTarget = { 0.0f, 0.0f, 0.0f };
		light.ShadowDistance = 30.0f;
		light.ShadowNear = 0.1f;
		light.ShadowFar = 200.0f;
		light.ShadowBias = 0.015f;
	}

	void GameSceneFactory::CreateStartEffect()
	{

	}

}
  
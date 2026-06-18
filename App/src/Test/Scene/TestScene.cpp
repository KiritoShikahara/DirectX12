#include "TestScene.h"

// コンポーネント
#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/sprite/SpriteComponent.h>
#include<ecs/component/camera/CameraComponent.h>
#include<ecs/component/Fbx/FbxComponent.h>
#include<ecs/component/Fbx/FbxAnimComponent.h>
#include<ecs/component/Light/LightComponent.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>
#include<ecs/component/skybox/SkyboxComponent.h>
#include<ecs/component/Text/TextComponent.h>
#include<ecs/component/Effect/EffectComponent.h>
#include<ecs/component/Shape/ShapeComponent.h>

// リソース
#include<graphics/Texture/TextureManager.h>
#include<graphics/PrimitiveModel/Resource/PrimitiveResourceManager.h>
#include<ecs/entity/EntityManager.h>
#include<graphics/Fbx/Resource/FbxResourceManager.h>
#include<audio/Manager/AudioManager.h>
#include<graphics/Effect/Manager/EffectManager.h>
#include<graphics/Texture/Texture.h>
#include<audio/Resource/AudioResourceManager.h>

// システム
#include<system/Window/Window.h>
#include<system/Scene/Factory/SceneFactory.h>

namespace scene
{
	void TestScene::Initialize()
	{
		LoadResource();
		CreateFbx();
		CreateField();
		CreateSprite();
		CreateCamera();
		CreateSound();
		CreateLight();
		CreateText();
		CreateEffect();
		CreateSkybox();
		CreateShape();
	}

	void TestScene::Finalize()
	{

	}


	void TestScene::LoadResource()
	{
		{
			auto& manager = graphics::FbxResourceManager::Get();
			auto res = manager.Load("Assets/Fbx/Faul.fbx.bin");
			bool ret = manager.LoadAnm("Assets/Fbx/Faul.fbx.bin", "Assets/Fbx/Animation/Attack_A.fbx.anm", "Attack_A");
			ret = manager.LoadAnm("Assets/Fbx/Faul.fbx.bin", "Assets/Fbx/Animation/Attack_B.fbx.anm", "Attack_B");
		}
		{
			auto& ResManager = audio::AudioResourceManager::Get();
			auto res = ResManager.GetResource("Assets/SE/TestSE.aud");
		}
	}

	void TestScene::CreateSprite()
	{
		auto texture = graphics::TextureManager::Get().GetOrLoad("Assets/Test/test.png");
		auto entity = ecs::EntityManager::Get().CreateEntity();
		auto& tr = ecs::EntityManager::Get().AddComponent<ecs::Transform>(entity);
		auto& sprite = ecs::EntityManager::Get().AddComponent<ecs::Sprite>(entity, texture);
		sprite.FillAmount = 1.f;
		sprite.Color = { 1,1,1,0.5 };
	}
	void TestScene::CreateFbx()
	{
		auto& manager = ecs::EntityManager::Get();
		auto& reg = manager.GetRegistry();

		auto res = graphics::FbxResourceManager::Get().Load("Assets/Fbx/Faul.fbx.bin");
		float scale = 0.2f;

		auto entity = manager.CreateEntity();
		auto& tr = manager.AddComponent<ecs::Transform>(entity);
		tr.SetScale(scale);
		tr.SetPosition(0, 10, 0);

		auto& fbx = manager.AddComponent<ecs::FbxComponent>(entity);
		fbx.Resource = res;
		fbx.CustomColor = { 1,1,1,1 };

		auto& anim = manager.AddComponent<ecs::FbxAnimComponent>(entity);
		anim.Play(*fbx.Resource, "Attack_A", true);

		reg.emplace<ecs::ColliderComponent>(entity, ecs::ColliderComponent::MakeBox({ 1,3,1 }));
		reg.emplace<ecs::RigidBodyComponent>(entity, ecs::RigidBodyComponent::MakeDynamic());
	}
	void TestScene::CreateSound()
	{
		auto& AudioManager = audio::AudioManager::Get();
		AudioManager.PlaySE("Assets/SE/TestSE.aud");
		AudioManager.PlayBGM("Assets/SE/TestBGM.aud");
	}
	void TestScene::CreateCamera()
	{
		auto& registry = ecs::EntityManager::Get().GetRegistry();
		entt::entity entity = ecs::EntityManager::Get().CreateEntity();

		auto& tr = registry.emplace<ecs::Transform>(entity);
		tr.SetPosition(0.0f, 1.0f, -100.0f);

		auto& cam = registry.emplace<ecs::CameraComponent>(entity);
		cam.IsMainCamera = true;
		cam.Fov = 60.0f;
		cam.Near = 0.1f;
		cam.Far = 1000.0f;
		cam.SetAspectRatioFromWindow(sys::Window::Get());
	}
	void TestScene::CreateLight()
	{
		auto& registry = ecs::EntityManager::Get().GetRegistry();
		entt::entity entity = ecs::EntityManager::Get().CreateEntity();

		registry.emplace<ecs::Transform>(entity);

		auto& light = registry.emplace<ecs::DirectionalLightComponent>(entity);
		light.Direction = { 0.3f, -1.0f, 0.5f }; // 斜め下向き
		light.Color = { 1.0f,  1.0f, 1.0f };
		light.Intensity = 1.0f;
		light.IsActive = true;
		light.CastShadow = true;
		light.ShadowRange = 50.0f;
		light.ShadowTarget = { 0.0f, 0.0f, 0.0f };
		light.ShadowDistance = 30.0f;
		light.ShadowNear = 0.1f;
		light.ShadowFar = 200.0f;
		light.ShadowBias = 0.015f;
	}
	void TestScene::CreateField()
	{
		auto& manager = ecs::EntityManager::Get();
		auto& registry = manager.GetRegistry();

		auto res = graphics::PrimitiveResourceManager::Get().GetResource("Field");
		float scale = 10;

		auto entity = manager.CreateEntity();
		auto& tr = manager.AddComponent<ecs::Transform>(entity);
		tr.SetScale(scale);

		auto& fbx = manager.AddComponent<ecs::FbxComponent>(entity);
		fbx.Resource = res;
		fbx.CustomColor = { 1,0,0,1 };

		registry.emplace<ecs::ColliderComponent>(entity,
			ecs::ColliderComponent::MakeBox({ 50.f, 0.5f, 50.f }));
		registry.emplace<ecs::RigidBodyComponent>(entity,
			ecs::RigidBodyComponent::MakeStatic());
	}
	void TestScene::CreateText()
	{
		auto& manager = ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();
		auto& text = manager.AddComponent<ecs::TextComponent>(entity);
		text.Text = L"Japan";
		text.Size = 64;
		text.Layer = 0;
		text.X = 200;
		text.Y = 200;
	}
	void TestScene::CreateEffect()
	{
		auto& manager = ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();
		auto& transform = manager.AddComponent<ecs::Transform>(entity);
		transform.SetScale(10);
		transform.SetPosition(100, 100, 0);

		auto& effect = manager.AddComponent<ecs::EffectComponent>(entity);
		effect.Asset = graphics::EffekseerManager::Get().GetEffect("Assets/Effect/Light3.efk");
		effect.IsLoop = true;
		effect.Effect.Play(effect.Asset, effect.Offset);
	}
	void TestScene::CreateSkybox()
	{
		auto& manager = ecs::EntityManager::Get();
		auto entity = manager.CreateEntity();
		auto& skybox = manager.AddComponent<ecs::SkyboxComponent>(entity);
		skybox.TexturePath = "Assets/Skybox/skybox.dds";
	}

	void TestScene::CreateShape()
	{
		auto& manager = ecs::EntityManager::Get();
		{
			auto entity = manager.CreateEntity();
			auto& transform = manager.AddComponent<ecs::Transform>(entity);
			transform.Set2DPosition(200, 200);

			auto& shape = manager.AddComponent<ecs::Shape>(entity);
			shape.Size = { 200,100 };
			shape.Type = ecs::ShapeType::Rect;
			shape.Color = graphics::Color::Red;
		}

		{
			auto entity = manager.CreateEntity();
			auto& transform = manager.AddComponent<ecs::Transform>(entity);
			transform.Set2DPosition(400, 200);

			auto& shape = manager.AddComponent<ecs::Shape>(entity);
			shape.Size = { 50,50};
			shape.Type = ecs::ShapeType::Circle;
			shape.Color = graphics::Color::Blue;
			shape.FType = ecs::FillType::Radial;
			shape.FillAmount = 1.0;
		}

		{
			auto entity = manager.CreateEntity();
			auto& transform = manager.AddComponent<ecs::Transform>(entity);
			transform.Set2DPosition(700, 200);

			auto& shape = manager.AddComponent<ecs::Shape>(entity);
			shape.Size = { 200,200 };
			shape.Type = ecs::ShapeType::Triangle;
			shape.Color = graphics::Color::Green;
		}

	}

	REGISTER_SCENE_AS(TestScene, "Test");
}


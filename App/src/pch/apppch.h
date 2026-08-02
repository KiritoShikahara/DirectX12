#pragma once



#include<ecs/entity/EntityManager.h>
#include<ecs/system/manager/IComponentSystem.h>
#include<ecs/system/manager/ComponentSystemManager.h>

#include<ecs/component/transform/TransformComponent.h>
#include<ecs/component/sprite/SpriteComponent.h>
#include<ecs/component/sprite/SpriteAnimationComponent.h>
#include<ecs/component/Shape/ShapeComponent.h>
#include<ecs/component/Text/TextComponent.h>
#include<ecs/component/camera/CameraComponent.h>
#include<ecs/component/Fbx/FbxComponent.h>
#include<ecs/component/Fbx/FbxAnimComponent.h>
#include<ecs/component/collider/ColliderComponent.h>
#include<ecs/component/rigidbody/RigidbodyComponent.h>
#include<ecs/component/Light/LightComponent.h>
#include<ecs/component/skybox/SkyboxComponent.h>
#include<ecs/component/Effect/EffectComponent.h>

#include<graphics/Texture/Texture.h>
#include<graphics/Texture/TextureManager.h>
#include<graphics/Fbx/Resource/FbxResourceManager.h>
#include<graphics/Effect/Manager/EffectManager.h>
#include<graphics/PrimitiveModel/Resource/PrimitiveResourceManager.h>

#include<audio/Resource/AudioResourceManager.h>
#include<audio/Manager/AudioManager.h>

#include<system/AssetPath/AssetPathManager.h>
#include<system/Logger/Logger.h>
#include<system/Scene/Factory/SceneFactory.h>
#include<system/Window/Window.h>
#include<system/Input/InputManager.h>
#include<system/Scene/Manager/SceneManager.h>
#include<ImGui/imgui.h>
#include<system/ImGui/ImGuiManager.h>
#include<system/Time/TimeManager.h>

#include<Data/Storage/Registry/DataRegistry.h>
#include<Data/Storage/Registry/ConfigRegistry.h>
#include<Data/Storage/Inspector/DataInspector.h>

#include<Utility/Singleton/Singleton.hpp>

#include<cmath>
#include<vector>
#include<string>
#include<filesystem>
#include<unordered_map>
#include<algorithm>

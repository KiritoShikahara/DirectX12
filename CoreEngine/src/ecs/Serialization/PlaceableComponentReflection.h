#pragma once

#include "ComponentReflection.h"
#include <ecs/component/transform/TransformComponent.h>
#include <ecs/component/Fbx/FbxComponent.h>
#include <ecs/component/Light/LightComponent.h>
#include <ecs/component/collider/ColliderComponent.h>
#include <ecs/component/rigidbody/RigidbodyComponent.h>
#include <ecs/component/sprite/SpriteComponent.h>

// Transform
ECS_REFLECT_BEGIN(ecs::Transform)
ECS_REFLECT_FIELD_ACCESSOR(Position, DirectX::XMFLOAT3, GetPosition, SetPosition)
ECS_REFLECT_FIELD_ACCESSOR(Rotation, DirectX::XMFLOAT4, GetRotation, SetRotation)
ECS_REFLECT_FIELD_ACCESSOR(Scale, DirectX::XMFLOAT3, GetScale, SetScale)
ECS_REFLECT_END()

// FbxComponent
ECS_REFLECT_BEGIN(ecs::FbxComponent)
ECS_REFLECT_FIELD(Layer)
ECS_REFLECT_FIELD(IsVisible)
ECS_REFLECT_FIELD(AutoPivot)
ECS_REFLECT_FIELD(PivotOffset)
ECS_REFLECT_FIELD(CustomColor)
ECS_REFLECT_END()

// DirectionalLightComponent
ECS_REFLECT_BEGIN(ecs::DirectionalLightComponent)
ECS_REFLECT_FIELD(Direction)
ECS_REFLECT_FIELD(Color)
ECS_REFLECT_FIELD(Intensity)
ECS_REFLECT_FIELD(IsActive)
ECS_REFLECT_FIELD(CastShadow)
ECS_REFLECT_FIELD(ShadowRange)
ECS_REFLECT_FIELD(ShadowNear)
ECS_REFLECT_FIELD(ShadowFar)
ECS_REFLECT_FIELD(ShadowTarget)
ECS_REFLECT_FIELD(ShadowDistance)
ECS_REFLECT_FIELD(ShadowBias)
ECS_REFLECT_END()

// PointLightComponent
ECS_REFLECT_BEGIN(ecs::PointLightComponent)
ECS_REFLECT_FIELD(Color)
ECS_REFLECT_FIELD(Intensity)
ECS_REFLECT_FIELD(Range)
ECS_REFLECT_FIELD(IsActive)
ECS_REFLECT_END()

// SpotLightComponent
ECS_REFLECT_BEGIN(ecs::SpotLightComponent)
ECS_REFLECT_FIELD(Direction)
ECS_REFLECT_FIELD(Color)
ECS_REFLECT_FIELD(Intensity)
ECS_REFLECT_FIELD(Range)
ECS_REFLECT_FIELD(InnerConeRad)
ECS_REFLECT_FIELD(OuterConeRad)
ECS_REFLECT_FIELD(IsActive)
ECS_REFLECT_END()

// ColliderComponent
ECS_REFLECT_BEGIN(ecs::ColliderComponent)
ECS_REFLECT_FIELD_ENUM(Shape)
ECS_REFLECT_FIELD(HalfExtent)
ECS_REFLECT_FIELD(Radius)
ECS_REFLECT_FIELD(HalfHeight)
ECS_REFLECT_FIELD(Offset)
ECS_REFLECT_END()

// RigidBodyComponent
ECS_REFLECT_BEGIN(ecs::RigidBodyComponent)
ECS_REFLECT_FIELD_ENUM(MotionType)
ECS_REFLECT_FIELD(Mass)
ECS_REFLECT_FIELD(Friction)
ECS_REFLECT_FIELD(Restitution)
ECS_REFLECT_FIELD(GravityFactor)
ECS_REFLECT_FIELD(LinearDamping)
ECS_REFLECT_FIELD(LockRotationX)
ECS_REFLECT_FIELD(LockRotationY)
ECS_REFLECT_FIELD(LockRotationZ)
ECS_REFLECT_FIELD(SyncRotation)
ECS_REFLECT_END()

// Sprite
ECS_REFLECT_BEGIN(ecs::Sprite)
ECS_REFLECT_FIELD(Pivot)
ECS_REFLECT_FIELD(Size)
ECS_REFLECT_FIELD(Layer)
ECS_REFLECT_FIELD(IsVisible)
ECS_REFLECT_END()
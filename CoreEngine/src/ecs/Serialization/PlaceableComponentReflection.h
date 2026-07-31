#pragma once

// エディタで配置可能な対象コンポーネントの ComponentTypeDescriptor<T> 特殊化を
// まとめたヘッダー。EditorSerialization / EditorUI(Inspector) の両方から include される。
//
// 対象は v1 で合意した「主要な配置系」のみ:
//   Transform / FbxComponent / DirectionalLight / PointLight / SpotLight /
//   ColliderComponent / RigidBodyComponent / Sprite
//
// FbxComponent::Resource / Sprite::Texture (アセットへの生ポインタ) と
// RigidBodyComponent::BodyID/IsBodyCreated (Jolt ハンドル・実行時状態) は
// reflection の対象外とし、別途 EditorSerialization 側でアセットキー文字列や
// 初期化フラグとして扱う。
// Sprite::Color は graphics::Color 型 (XMFLOAT2/3/4 と一致しない) のため、
// IComponentFieldVisitor の対応型を増やさずに済むよう reflection 対象外とする
// (配置直後の見た目確認用途では既定色で十分なため)。

#include "ComponentReflection.h"

#include <ecs/component/transform/TransformComponent.h>
#include <ecs/component/Fbx/FbxComponent.h>
#include <ecs/component/Light/LightComponent.h>
#include <ecs/component/collider/ColliderComponent.h>
#include <ecs/component/rigidbody/RigidbodyComponent.h>
#include <ecs/component/sprite/SpriteComponent.h>

// ── Transform (private フィールドなので Getter/Setter 経由) ──────────────
ECS_REFLECT_BEGIN(ecs::Transform)
	ECS_REFLECT_FIELD_ACCESSOR(Position, DirectX::XMFLOAT3, GetPosition, SetPosition)
	ECS_REFLECT_FIELD_ACCESSOR(Rotation, DirectX::XMFLOAT4, GetRotation, SetRotation)
	ECS_REFLECT_FIELD_ACCESSOR(Scale, DirectX::XMFLOAT3, GetScale, SetScale)
ECS_REFLECT_END()

// ── FbxComponent (Resource は対象外、EditorSerialization 側で AssetKey 経由で解決) ──
ECS_REFLECT_BEGIN(ecs::FbxComponent)
	ECS_REFLECT_FIELD(Layer)
	ECS_REFLECT_FIELD(IsVisible)
	ECS_REFLECT_FIELD(AutoPivot)
	ECS_REFLECT_FIELD(PivotOffset)
	ECS_REFLECT_FIELD(CustomColor)
ECS_REFLECT_END()

// ── DirectionalLightComponent ──────────────────────────────────────────
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

// ── PointLightComponent ────────────────────────────────────────────────
ECS_REFLECT_BEGIN(ecs::PointLightComponent)
	ECS_REFLECT_FIELD(Color)
	ECS_REFLECT_FIELD(Intensity)
	ECS_REFLECT_FIELD(Range)
	ECS_REFLECT_FIELD(IsActive)
ECS_REFLECT_END()

// ── SpotLightComponent ─────────────────────────────────────────────────
ECS_REFLECT_BEGIN(ecs::SpotLightComponent)
	ECS_REFLECT_FIELD(Direction)
	ECS_REFLECT_FIELD(Color)
	ECS_REFLECT_FIELD(Intensity)
	ECS_REFLECT_FIELD(Range)
	ECS_REFLECT_FIELD(InnerConeRad)
	ECS_REFLECT_FIELD(OuterConeRad)
	ECS_REFLECT_FIELD(IsActive)
ECS_REFLECT_END()

// ── ColliderComponent ──────────────────────────────────────────────────
ECS_REFLECT_BEGIN(ecs::ColliderComponent)
	ECS_REFLECT_FIELD_ENUM(Shape)
	ECS_REFLECT_FIELD(HalfExtent)
	ECS_REFLECT_FIELD(Radius)
	ECS_REFLECT_FIELD(HalfHeight)
	ECS_REFLECT_FIELD(Offset)
ECS_REFLECT_END()

// ── RigidBodyComponent (BodyID / IsBodyCreated / MoveVelocity / HasMoveRequest は対象外) ──
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

// ── Sprite (Color は非対応型のため対象外、Texture は AssetKey 経由で解決) ──
ECS_REFLECT_BEGIN(ecs::Sprite)
	ECS_REFLECT_FIELD(Pivot)
	ECS_REFLECT_FIELD(Size)
	ECS_REFLECT_FIELD(Layer)
	ECS_REFLECT_FIELD(IsVisible)
ECS_REFLECT_END()

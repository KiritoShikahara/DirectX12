#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace ecs
{
	/// <summary>
	/// 指向性ライトのシャドウ注視点(DirectionalLightComponent::ShadowTarget)を
	/// プレイヤー位置(XZのみ)へ追従させる。
	///
	/// フィールドは約2000x2000ユニットあるのに対し、Shadow Mapの正射影範囲(ShadowRange)は
	/// 影の解像度を保つため100前後に絞ってある。ShadowTargetをワールド原点に固定していると、
	/// プレイヤーが少し移動しただけで自身のいる場所がShadow Mapの範囲外に出てしまい、
	/// SampleShadowPCF(FbxShader.hlsli)が範囲外を「常に影なし」として扱うため、
	/// フィールドの大部分で影が機能せず光の陰影が付かない(=フィールド全体に光が回っていない
	/// ように見える)不具合になっていた。
	///
	/// 注視点をプレイヤー中心へ追従させることで、影の解像度を落とさずに
	/// 「プレイヤーが今いる場所」では常にシャドウ/陰影が正しく機能するようにする
	/// (CameraPlayerFollowSystemと同じ、プレイヤー追従パターン)。
	/// PostUpdateフェーズに登録し、Engine::Render()直前のLightSystem::Update
	/// (LightViewProjの計算)より前にShadowTargetを確定させること。
	/// </summary>
	class DirLightFollowSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;
	};
}

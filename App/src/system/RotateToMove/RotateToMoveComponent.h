#pragma once
#include<Utility/Export/Export.h>

namespace ecs
{
	///<summary>
	///RigidBodyComponent::MoveVelocityの水平方向に応じてTransformを回転させるための汎用コンポーネント。プレイヤー・敵共に流用可能
	///</summary>
	struct ENGINE_API RotateToMoveComponent
	{
		///<summary>
		///1秒あたりの回転速度、度
		///</summary>
		float RotationSpeedDeg = 720.0f;

		///<summary>
		///trueで補間せず即座に目標角度へ向く
		///</summary>
		bool InstantRotate = false;
	};
}

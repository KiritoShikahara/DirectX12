#pragma once

#if defined(_DEBUG)
namespace Editor
{
	// カメラ用の定数
	namespace Camera
	{
		float MoveSpeed = 10.0f;	// カメラの移動速度
		float RotateSpeed = 0.003f; // カメラの回転速度
	}

}
#endif

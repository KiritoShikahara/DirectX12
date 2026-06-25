#pragma once

#include<cstdint>

namespace ecs
{
	/// <summary>
	/// プレイヤー生成時に使用するコンテキスト 
	/// </summary>
	struct CreatePlayerContext
	{
		uint32_t SelectSpellID;
	};

	class GameSceneFactory
	{
	public:
		// 背景音
		static void CreateBGM();

		// 状態管理
		static void CreateStateObject();

		// 地面
		static void CreateGround();

		// プレイヤー
		static void CreatePlayer(const CreatePlayerContext& Context);

		// カメラ
		static void CreateCamera();

		// ディレクションライト
		static void CreateDirLight();

		// 開始時のエフェクト生成
		static void CreateStartEffect();
	};
}


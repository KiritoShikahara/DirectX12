#pragma once

#include<system/Scene/IScene.h>

namespace scene
{
	class GameScene : public ::sys::IScene
	{
	public:
		GameScene() = default;
		GameScene(uint32_t SpellID);

		virtual void Initialize()override;
		virtual void Finalize()override;
	private:
		// データ読み込み
		void LoadData();

		// 背景音
		void CreateBGM();

		// 状態管理
		void CreateStateObject();

		// 地面
		void CreateGround();

		// プレイヤー
		void CreatePlayer();

		// カメラ
		void CreateCamera();

	private:
		/// <summary>
		/// 選択されたスペルのID
		/// </summary>
		uint32_t mSpellID = 0;
	};
}



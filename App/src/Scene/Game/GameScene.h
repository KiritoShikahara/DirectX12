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

		// リソース読み込み
		void LoadResource();

		// エンティティの生成
		void CreateEntitys();
	private:
		/// <summary>
		/// 選択されたスペルのID
		/// </summary>
		uint32_t mSpellID = 1001;
	};
}



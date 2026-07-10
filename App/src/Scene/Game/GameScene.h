#pragma once

#include<system/Scene/IScene.h>
#include<system/Enemy/Status/EnemyStatusDebugPanel.h>

#include<memory>

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

		// システムの登録
		void CreateUserSystem();

		// エンティティの生成
		void CreateEntitys();

		// デバック処理呼び出し
		void DebugInitialize();
		// デバック処理終了
		void DebugFinalize();
	private:
		/// <summary>
		/// 選択されたスペルのID
		/// </summary>
		uint32_t mSpellID = 1001;

		std::unique_ptr<debug::EnemyStatusDebugPanel> mEnemyStatusDebugPanel;
	};
}



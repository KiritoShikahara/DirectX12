#pragma once

#include<system/Scene/IScene.h>
#include<system/Enemy/Status/EnemyStatusDebugPanel.h>
#include<Scene/Game/Debug/GameStatusDebugPanel.h>
#include<Scene/Game/Wave/WaveDebugPanel.h>
#include<system/Player/Weapon/SingleShot/SingleShotWeaponDebugPanel.h>
#include<Scene/Game/Debug/WeaponInventoryDebugPanel.h>
#include<system/Player/Ultimate/UltimateDebugPanel.h>
#include<Data/Save/PlayerSaveDebugPanel.h>

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

		// 各武器/必殺技のEffekseerエフェクト素材をロード画面中に先読みする
		// (プレイ中の初回発動時にテクスチャ読み込みが走り、他エフェクトが一瞬乱れるのを防ぐ)
		void PreloadWeaponEffects();

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
		std::unique_ptr<debug::GameStatusDebugPanel> mGameStatusDebugPanel;
		std::unique_ptr<debug::WaveDebugPanel> mWaveDebugPanel;
		std::unique_ptr<debug::SingleShotWeaponDebugPanel> mSingleShotWeaponDebugPanel;
		std::unique_ptr<debug::WeaponInventoryDebugPanel> mWeaponInventoryDebugPanel;
		std::unique_ptr<debug::UltimateDebugPanel> mUltimateDebugPanel;
		std::unique_ptr<debug::PlayerSaveDebugPanel> mPlayerSaveDebugPanel;
	};
}



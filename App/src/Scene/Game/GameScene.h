#pragma once

#include<system/Scene/IScene.h>
#include<system/Enemy/Status/EnemyStatusDebugPanel.h>
#include<Scene/Game/Debug/GameStatusDebugPanel.h>
#include<Scene/Game/Wave/WaveDebugPanel.h>
#include<system/Player/Perk/PerkDebugPanel.h>
#include<system/Player/Weapon/WeaponMasterDataDebugPanel.h>
#include<Scene/Game/Debug/WeaponInventoryDebugPanel.h>
#include<system/Player/Ultimate/UltimateDebugPanel.h>
#include<Data/Save/PlayerSaveDebugPanel.h>
#include<Data/Settings/GameSettingsDebugPanel.h>
#include<system/StatusUpgrade/StatUpgradeDebugPanel.h>
#include<system/Effect/EffectAssetDebugPanel.h>
#include<system/Enemy/Status/BossDebugPanel.h>

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
		void LoadData();

		void LoadResource();

		///<summary>
		///各武器/必殺技のエフェクト素材をロード画面中に先読みする。初回発動時のテクスチャ読み込みで他エフェクトが乱れるのを防ぐ
		///</summary>
		void PreloadWeaponEffects();

		void CreateUserSystem();

		void CreateEntitys();

		void DebugInitialize();
		void DebugFinalize();
	private:
		///<summary>
		///選択されたスペルのID
		///</summary>
		uint32_t mSpellID = 1001;

		std::unique_ptr<debug::EnemyStatusDebugPanel> mEnemyStatusDebugPanel;
		std::unique_ptr<debug::GameStatusDebugPanel> mGameStatusDebugPanel;
		std::unique_ptr<debug::WaveDebugPanel> mWaveDebugPanel;
		std::unique_ptr<debug::PerkDebugPanel> mPerkDebugPanel;
		std::unique_ptr<debug::WeaponMasterDataDebugPanel> mWeaponMasterDataDebugPanel;
		std::unique_ptr<debug::WeaponInventoryDebugPanel> mWeaponInventoryDebugPanel;
		std::unique_ptr<debug::UltimateDebugPanel> mUltimateDebugPanel;
		std::unique_ptr<debug::PlayerSaveDebugPanel> mPlayerSaveDebugPanel;
		std::unique_ptr<debug::GameSettingsDebugPanel> mGameSettingsDebugPanel;
		std::unique_ptr<debug::StatUpgradeDebugPanel> mStatUpgradeDebugPanel;
		std::unique_ptr<debug::EffectAssetDebugPanel> mEffectAssetDebugPanel;
		std::unique_ptr<debug::BossDebugPanel> mBossDebugPanel;
	};
}



#pragma once

#include<system/Scene/IScene.h>
#include<system/Enemy/Status/EnemyStatusDebugPanel.h>
#include<Scene/Game/Debug/GameStatusDebugPanel.h>
#include<Scene/Game/Wave/WaveDebugPanel.h>
#include<system/Player/Perk/PerkDebugPanel.h>
#include<system/Player/Weapon/SingleShot/SingleShotWeaponDebugPanel.h>
#include<Scene/Game/Debug/WeaponInventoryDebugPanel.h>
#include<system/Player/Ultimate/UltimateDebugPanel.h>
#include<Data/Save/PlayerSaveDebugPanel.h>
#include<system/Player/Weapon/Ricochet/RicochetWeaponDebugPanel.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackWeaponDebugPanel.h>
#include<system/Player/Weapon/BoneSpear/BoneSpearWeaponDebugPanel.h>
#include<system/Player/Weapon/ChainLightning/ChainLightningWeaponDebugPanel.h>
#include<system/Player/Weapon/Cleave/CleaveWeaponDebugPanel.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeWeaponDebugPanel.h>
#include<system/Player/Weapon/Homing/HomingMissileWeaponDebugPanel.h>
#include<system/Player/Weapon/Meteor/MeteorWeaponDebugPanel.h>
#include<system/Player/Weapon/Nova/NovaWeaponDebugPanel.h>
#include<system/Player/Weapon/Orbit/OrbitWeaponDebugPanel.h>
#include<system/Player/Weapon/VoidBeam/VoidBeamWeaponDebugPanel.h>
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
		std::unique_ptr<debug::PerkDebugPanel> mPerkDebugPanel;
		std::unique_ptr<debug::SingleShotWeaponDebugPanel> mSingleShotWeaponDebugPanel;
		std::unique_ptr<debug::WeaponInventoryDebugPanel> mWeaponInventoryDebugPanel;
		std::unique_ptr<debug::UltimateDebugPanel> mUltimateDebugPanel;
		std::unique_ptr<debug::PlayerSaveDebugPanel> mPlayerSaveDebugPanel;
		std::unique_ptr<debug::RicochetWeaponDebugPanel> mRicochetWeaponDebugPanel;
		std::unique_ptr<debug::AreaAttackWeaponDebugPanel> mAreaAttackWeaponDebugPanel;
		std::unique_ptr<debug::BoneSpearWeaponDebugPanel> mBoneSpearWeaponDebugPanel;
		std::unique_ptr<debug::ChainLightningWeaponDebugPanel> mChainLightningWeaponDebugPanel;
		std::unique_ptr<debug::CleaveWeaponDebugPanel> mCleaveWeaponDebugPanel;
		std::unique_ptr<debug::FlickerStrikeWeaponDebugPanel> mFlickerStrikeWeaponDebugPanel;
		std::unique_ptr<debug::HomingMissileWeaponDebugPanel> mHomingMissileWeaponDebugPanel;
		std::unique_ptr<debug::MeteorWeaponDebugPanel> mMeteorWeaponDebugPanel;
		std::unique_ptr<debug::NovaWeaponDebugPanel> mNovaWeaponDebugPanel;
		std::unique_ptr<debug::OrbitWeaponDebugPanel> mOrbitWeaponDebugPanel;
		std::unique_ptr<debug::VoidBeamWeaponDebugPanel> mVoidBeamWeaponDebugPanel;
		std::unique_ptr<debug::StatUpgradeDebugPanel> mStatUpgradeDebugPanel;
		std::unique_ptr<debug::EffectAssetDebugPanel> mEffectAssetDebugPanel;
		std::unique_ptr<debug::BossDebugPanel> mBossDebugPanel;
	};
}



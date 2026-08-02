#include "apppch.h"
#include "GameScene.h"

#include"../macros.h"
#include"Factory/GameSceneFactory.h"

#include<system/RotateToMove/RotateToMoveSystem.h>
#include<scene/Game/State/GameStateSystem.h>

// カメラ
#include<system/CameraFollow/CameraPlayerFollowSystem.h>
#include<system/Light/DirLightFollowSystem.h>

// UI
#include<system/Player/UI/PlayerHpBarSystem.h>
#include<system/Player/UI/PlayerUltimateGaugeSystem.h>
#include<system/Player/UI/WeaponIconBarSystem.h>
#include<system/Player/UI/PlayerExpBarSystem.h>
#include<Utility/config/DebugConfig.h> // DEV_TOOL_ENABLED(Debug/Develop両方で有効)を参照するため直接include
#include<system/GlowAnimation/SpriteGlowSystem.h>
#include<system/UI/DamageNumber/DamageNumberSystem.h>

// プレイヤー
#include<system/Player/State/PlayerStateSystem.h>
#include<system/Player/InputSystem/PlayerInputSystem.h>
#include<system/Player/MovementSystem/PlayerMovementSystem.h>
#include<system/Animation/LocomotionAnimationSystem.h>
#include<system/Player/AimSysten/PlayerAimSystem.h>
#include<system/Player/Boundary/PlayerBoundaryClampSystem.h>

// 武器
#include<system/Player/Weapon/SingleShot/SingleShotWeaponSystem.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackWeaponSystem.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackAutoStrikeSystem.h>
#include<system/Player/Weapon/AreaAttack/AreaAttackHazardSystem.h>
#include<system/Player/Weapon/Orbit/OrbitWeaponSystem.h>
#include<system/Player/Weapon/Nova/NovaWeaponSystem.h>
#include<system/Player/Weapon/Homing/HomingMissileWeaponSystem.h>
#include<system/Player/Weapon/Homing/HomingMissileSteeringSystem.h>
#include<system/Player/Weapon/ChainLightning/ChainLightningWeaponSystem.h>
#include<system/Player/Weapon/Meteor/MeteorWeaponSystem.h>
#include<system/Player/Weapon/VoidBeam/VoidBeamWeaponSystem.h>
#include<system/Player/Weapon/BoneSpear/BoneSpearWeaponSystem.h>
#include<system/Player/Weapon/Cleave/CleaveWeaponSystem.h>
#include<system/Player/Weapon/FlickerStrike/FlickerStrikeWeaponSystem.h>
#include<system/Player/Weapon/Ricochet/RicochetWeaponSystem.h>
#include<system/Player/Ultimate/PlayerUltimateSystem.h>
#include<system/Player/Weapon/Projectile/ProjectileMovementSystem.h>
#include<system/Player/Weapon/Projectile/ProjectileCollisionSystem.h>
#include<system/Effect/TemporaryLifetimeSystem.h>

// 敵
#include<system/Enemy/Move/EnemyChaseSystem.h>
#include<system/Enemy/Death/EnemyDeathSystem.h>
#include<system/Enemy/Knockback/EnemyKnockbackSystem.h>
#include<system/Enemy/Status/EnemySlowStatusSystem.h>

// パーク
#include<system/Player/Perk/PerkSelectSystem.h>
#include<system/Options/OptionsMenuSystem.h>

// リザルト
#include<Scene/Game/Result/ResultSystem.h>

// ウェーブ
#include<Scene/Game/Wave/WaveComponent.h>
#include<Scene/Game/Wave/EnemySpawnSystem.h>
#include<Scene/Game/Wave/WaveTimerUiSystem.h>

// ダメージ
#include<system/Damage/PlayerContactDamage/PlayerContactDamageSystem.h>
#include<system/Player/Status/PlayerRegenSystem.h>

// エフェクト先読み
#include<system/Effect/EffectSpawnUtility.h>

// データ
#include<Data/Enemy/EnemyData.h>
#include<Data/Enemy/BossData.h>
#include<Data/Effect/EffectAssetData.h>
#include<Data/Weapon/SingleShotWeaponData.h>
#include<Data/Weapon/AreaAttackWeaponData.h>
#include<Data/Weapon/OrbitWeaponData.h>
#include<Data/Weapon/NovaWeaponData.h>
#include<Data/Weapon/HomingMissileWeaponData.h>
#include<Data/Weapon/ChainLightningWeaponData.h>
#include<Data/Weapon/MeteorWeaponData.h>
#include<Data/Weapon/VoidBeamWeaponData.h>
#include<Data/Weapon/BoneSpearWeaponData.h>
#include<Data/Weapon/CleaveWeaponData.h>
#include<Data/Weapon/FlickerStrikeWeaponData.h>
#include<Data/Weapon/RicochetWeaponData.h>
#include<Data/Ultimate/UltimateData.h>
#include<Data/StatUpgrade/StatUpgradeData.h>
#include<Data/Save/PlayerSaveData.h>
#include<Data/Wave/WaveData.h>
#include<Data/Perk/PerkData.h>

// デバッグ

namespace scene
{
	GameScene::GameScene(uint32_t SpellID)
		:mSpellID(SpellID)
	{
	}

	void GameScene::Initialize()
	{

		// データ
		LoadData();

		// システム
		CreateUserSystem();

		// リソース
		LoadResource();

		// 各武器/必殺技のエフェクト素材をロード画面中に先読みする
		PreloadWeaponEffects();

		// エンティティ生成
		CreateEntitys();

#if DEV_TOOL_ENABLED
		// デバッグ
		this->DebugInitialize();


#endif // _DEBUG

		DEBUG_LOG(::sys::eLogLevel::Log, "Game Scene.");
	}

	void GameScene::Finalize()
	{
		
#if DEV_TOOL_ENABLED
			// デバッグ
			this->DebugFinalize();


#endif // _DEBUG
	}

	void GameScene::LoadData()
	{
		auto id = mSpellID;

		// データ読み込み
		// DataRegistry はプロセス全体で1つのシングルトンのため、
		// GameScene が再初期化される(エディタの Stop によるシーン再構築等)場合、
		// 二重登録で RegistryBase の assert に落ちないようにガードする。
		auto& dataRegistry = data::DataRegistry::Get();
		if (!dataRegistry.IsRegistered<data::EnemyData>())
		{
			dataRegistry.Register<data::EnemyData>("Assets/Data/Enemy/EnemyData.csv");
		}
		if (!dataRegistry.IsRegistered<data::BossData>())
		{
			dataRegistry.Register<data::BossData>("Assets/Data/Enemy/BossData.csv");
		}
		// エフェクト素材ID→パスの解決テーブル。各武器データのXxxEffectIdsをResolveEffectIdsで
		// 解決する際に参照するため、武器データより先にロードされている必要がある
		// (LoadAll()が全登録後に一括ロードするため、実際の登録順はここでなくてもよいが、
		// 依存関係を明示するためにここへ置く)。
		if (!dataRegistry.IsRegistered<data::EffectAssetData>())
		{
			dataRegistry.Register<data::EffectAssetData>("Assets/Data/Effect/EffectAssetData.csv");
		}
		if (!dataRegistry.IsRegistered<data::SingleShotWeaponData>())
		{
			dataRegistry.Register<data::SingleShotWeaponData>("Assets/Data/Weapon/SingleShotWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::AreaAttackWeaponData>())
		{
			dataRegistry.Register<data::AreaAttackWeaponData>("Assets/Data/Weapon/AreaAttackWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::OrbitWeaponData>())
		{
			dataRegistry.Register<data::OrbitWeaponData>("Assets/Data/Weapon/OrbitWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::NovaWeaponData>())
		{
			dataRegistry.Register<data::NovaWeaponData>("Assets/Data/Weapon/NovaWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::HomingMissileWeaponData>())
		{
			dataRegistry.Register<data::HomingMissileWeaponData>("Assets/Data/Weapon/HomingMissileWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::ChainLightningWeaponData>())
		{
			dataRegistry.Register<data::ChainLightningWeaponData>("Assets/Data/Weapon/ChainLightningWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::MeteorWeaponData>())
		{
			dataRegistry.Register<data::MeteorWeaponData>("Assets/Data/Weapon/MeteorWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::VoidBeamWeaponData>())
		{
			dataRegistry.Register<data::VoidBeamWeaponData>("Assets/Data/Weapon/VoidBeamWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::BoneSpearWeaponData>())
		{
			dataRegistry.Register<data::BoneSpearWeaponData>("Assets/Data/Weapon/BoneSpearWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::CleaveWeaponData>())
		{
			dataRegistry.Register<data::CleaveWeaponData>("Assets/Data/Weapon/CleaveWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::FlickerStrikeWeaponData>())
		{
			dataRegistry.Register<data::FlickerStrikeWeaponData>("Assets/Data/Weapon/FlickerStrikeWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::RicochetWeaponData>())
		{
			dataRegistry.Register<data::RicochetWeaponData>("Assets/Data/Weapon/RicochetWeaponData.csv");
		}
		if (!dataRegistry.IsRegistered<data::UltimateData>())
		{
			dataRegistry.Register<data::UltimateData>("Assets/Data/Ultimate/UltimateData.csv");
		}
		if (!dataRegistry.IsRegistered<data::WaveData>())
		{
			dataRegistry.Register<data::WaveData>("Assets/Data/Wave/WaveData.csv");
		}
		if (!dataRegistry.IsRegistered<data::StatUpgradeData>())
		{
			dataRegistry.Register<data::StatUpgradeData>("Assets/Data/StatUpgrade/StatUpgradeData.csv");
		}
		if (!dataRegistry.IsRegistered<data::PerkData>())
		{
			dataRegistry.Register<data::PerkData>("Assets/Data/Perk/PerkData.csv");
		}
		dataRegistry.LoadAll();

		// プレイヤーの永続的な進行状況(ゴールド・ステータス強化レベル)。CSV/DBのマスタデータとは
		// 別系統(ConfigManager<T>によるJSON永続化)のため、DataRegistryとは別に読み込む
		data::EnsurePlayerSaveDataLoaded();
	}

	void GameScene::LoadResource()
	{
		auto& manager = ::graphics::FbxResourceManager::Get();

		// Field
		manager.Load("Assets/Fbx/Field/Field.fbx.bin");

		// Player
		auto playerPath = "Assets/Fbx/Faul/Faul.fbx.bin";
		manager.Load(playerPath);
		/// アニメーション

		// Enemy

		// Texture

	}

	/// <summary>
	/// 各武器/必殺技のマスタデータが参照するエフェクト素材を、ロード画面中にまとめて
	/// EffekseerManagerのキャッシュへ読み込んでおく。読み込みを各武器の初回発動まで
	/// 遅延させると、プレイ中に初めて発動した瞬間にテクスチャ読み込みが走り、その間の
	/// 数フレームだけ他の再生中エフェクトの描画が乱れる(四角形のポリゴンが一瞬見える等)
	/// ことがあるため、事前に読み込んでおくことでこれを避ける。
	/// ecs::effectutil::PreloadAllEffectPathFields<T>()がリフレクション(REFLECT_FIELD)経由で
	/// "EffectPath"で終わる文字列フィールドを自動的に見つけて読み込むため、既存フィールドへの
	/// エフェクトパス追加はここへの追記が不要。新しい武器種別(マスタデータ型)を追加した場合のみ
	/// 1行追記すること。
	/// </summary>
	void GameScene::PreloadWeaponEffects()
	{
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::SingleShotWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::AreaAttackWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::OrbitWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::NovaWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::HomingMissileWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::ChainLightningWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::MeteorWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::VoidBeamWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::BoneSpearWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::CleaveWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::FlickerStrikeWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::RicochetWeaponData).GetAll());
		ecs::effectutil::PreloadAllEffectPathFields(DATA_MGR(data::UltimateData).GetAll());

		// マスタデータに属さない固定演出(敵撃破・パーク確定)。EnemyDeathSystem/PerkSelectSystemの
		// パス文字列と一致させること
		ecs::effectutil::PreloadEffect("Assets/Effect/AttackHit.efk");
		ecs::effectutil::PreloadEffect("Assets/Effect/Herald.efk");
	}

	void GameScene::CreateUserSystem()
	{
		auto& manager = ::ecs::ComponentSystemManager::Get();
		manager.AddUserSystem<::ecs::PlayerInputSystem>(::ecs::eUpdatePhase::PreUpdate);
		manager.AddUserSystem<::sys::PlayerAimSystem>(::ecs::eUpdatePhase::PreUpdate);
		manager.AddUserSystem<::ecs::PlayerStateSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerMovementSystem>(::ecs::eUpdatePhase::Update);
		// スロウ状態(EnemySlowStatusComponent)の期限切れ解除はEnemyChaseSystem(速度参照)より前に置く
		manager.AddUserSystem<::ecs::EnemySlowStatusSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::EnemyChaseSystem>(::ecs::eUpdatePhase::Update);
		// 移動状態(IsMoving)に応じてIdle/Runへ切り替える。Player/Enemy両方のIsMovingが
		// 確定した後(PlayerMovementSystem/EnemyChaseSystemの後)に置き、
		// engine側のgraphics::FbxAnimSystem(時間進行・ボーン計算)より前に目的クリップを決める。
		manager.AddUserSystem<::ecs::LocomotionAnimationSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::EnemySpawnSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::WaveTimerUiSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerHpBarSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerUltimateGaugeSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::WeaponIconBarSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerExpBarSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerContactDamageSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::PlayerRegenSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::SingleShotWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::AreaAttackWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::AreaAttackAutoStrikeSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::AreaAttackHazardSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::OrbitWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::NovaWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::HomingMissileWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::HomingMissileSteeringSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::ChainLightningWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::MeteorWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::VoidBeamWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::BoneSpearWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::CleaveWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::FlickerStrikeWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::RicochetWeaponSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::ProjectileCollisionSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::ProjectileMovementSystem>(::ecs::eUpdatePhase::Update);
		manager.AddUserSystem<::ecs::TemporaryLifetimeSystem>(::ecs::eUpdatePhase::Update);
		// ノックバック処理(Cleave)はEnemyChaseSystemより前に実行し、
		// 同一フレーム内でMoveVelocityの上書き合戦にならないようにする
		manager.AddUserSystem<::ecs::EnemyKnockbackSystem>(::ecs::eUpdatePhase::Update);
		// ダメージ計算(Update)が終わった後にHP0の敵をまとめて破棄する
		manager.AddUserSystem<::ecs::EnemyDeathSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::RotateToMoveSystem>(::ecs::eUpdatePhase::PostUpdate);
		// PlayerUltimateSystemは必殺技演出中、CameraOverrideComponentへカメラ位置のリクエストを
		// 書き込むだけ(Transformは直接書き換えない)。そのリクエストを同一フレーム内で反映するため、
		// リクエストを消費して実際にTransformへ書き込むCameraPlayerFollowSystemより前段に置く
		manager.AddUserSystem<::ecs::PlayerUltimateSystem>(::ecs::eUpdatePhase::PostUpdate);
		// このフレーム中のテレポート(Flicker Strikeのワープ・必殺技のFinishAndExplode)を
		// まとめて境界内へクランプしてから、CameraPlayerFollowSystemに正しい位置を読ませる
		manager.AddUserSystem<::ecs::PlayerBoundaryClampSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::CameraPlayerFollowSystem>(::ecs::eUpdatePhase::PostUpdate);
		// LightSystem::Update(Engine.cpp、LightViewProjの計算)より前にShadowTargetを
		// 確定させる必要があるため、PostUpdateフェーズの中で(Renderより前であれば)登録する
		manager.AddUserSystem<::ecs::DirLightFollowSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::DamageNumberSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::sys::GameStateSystem>(::ecs::eUpdatePhase::PostUpdate);
		// GameStateSystemの後段に置くことで、PerkSelect/Resultへ遷移した同一フレームでUIを生成できる
		manager.AddUserSystem<::ecs::PerkSelectSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::ResultSystem>(::ecs::eUpdatePhase::PostUpdate);
		manager.AddUserSystem<::ecs::SpriteGlowSystem>(::ecs::eUpdatePhase::PostUpdate);
		// 設定メニュー(Escape / パッドのMenuボタンで開閉)。開いている間はTimeScaleを0にするため、
		// 他システムが状態を更新し終えた後段に置く
		manager.AddUserSystem<::ecs::OptionsMenuSystem>(::ecs::eUpdatePhase::PostUpdate);
	}

	void GameScene::CreateEntitys()
	{
		// 状態
		::ecs::GameSceneFactory::CreateStateController();
		// BGM
		::ecs::GameSceneFactory::CreateBGM();
		// ディレクションライト
		::ecs::GameSceneFactory::CreateDirLight();
		// 地面
		::ecs::GameSceneFactory::CreateGround();
		// フィールド外へ出られないようにする見えない境界壁
		::ecs::GameSceneFactory::CreateFieldBoundary();
		// 空(Skybox)。フィールド外側が虚無に見えないようにする
		::ecs::GameSceneFactory::CreateSkybox();
		// プレイヤー
		::ecs::GameSceneFactory::CreatePlayer(::ecs::CreatePlayerContext{ mSpellID });
		// カメラ
		::ecs::GameSceneFactory::CreateCamera();
		// UI
		::ecs::GameSceneFactory::CreateUI();

		// 敵はEnemySpawnSystemが継続的に生成するため、ここでの固定生成は行わない
	}

	void GameScene::DebugInitialize()
	{
		mEnemyStatusDebugPanel = std::make_unique<debug::EnemyStatusDebugPanel>();
		mGameStatusDebugPanel = std::make_unique<debug::GameStatusDebugPanel>();
		mWaveDebugPanel = std::make_unique<debug::WaveDebugPanel>();
		mPerkDebugPanel = std::make_unique<debug::PerkDebugPanel>();
		mWeaponMasterDataDebugPanel = std::make_unique<debug::WeaponMasterDataDebugPanel>();
		mWeaponInventoryDebugPanel = std::make_unique<debug::WeaponInventoryDebugPanel>();
		mUltimateDebugPanel = std::make_unique<debug::UltimateDebugPanel>();
		mPlayerSaveDebugPanel = std::make_unique<debug::PlayerSaveDebugPanel>("GameScene_PlayerSaveDebug");
		mStatUpgradeDebugPanel = std::make_unique<debug::StatUpgradeDebugPanel>();
		mEffectAssetDebugPanel = std::make_unique<debug::EffectAssetDebugPanel>();
		mBossDebugPanel = std::make_unique<debug::BossDebugPanel>();
	}

	void GameScene::DebugFinalize()
	{
		mEnemyStatusDebugPanel.reset();
		mGameStatusDebugPanel.reset();
		mWaveDebugPanel.reset();
		mPerkDebugPanel.reset();
		mWeaponMasterDataDebugPanel.reset();
		mWeaponInventoryDebugPanel.reset();
		mUltimateDebugPanel.reset();
		mPlayerSaveDebugPanel.reset();
		mStatUpgradeDebugPanel.reset();
		mEffectAssetDebugPanel.reset();
		mBossDebugPanel.reset();
	}


	REGISTER_SCENE_AS(GameScene, GAME_SCENE_NAME);

}



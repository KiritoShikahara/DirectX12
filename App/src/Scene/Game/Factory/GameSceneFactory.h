#pragma once

#include<cstdint>
#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Status/PlayerStatusComponent.h>

namespace ecs
{
	///<summary>
	///プレイヤー生成時に使用するコンテキスト
	///</summary>
	struct CreatePlayerContext
	{
		uint32_t SelectSpellID;
	};

	///<summary>
	///ボス階級。data::BossDataのIdと対応する。Noneは通常の敵でBossData参照を行わない
	///</summary>
	enum class eBossTier
	{
		None = -1,
		Mini = 0,   // 小ボス、周期的に複数回出現
		Mid = 1,    // 中ボス、1回だけ出現
		Final = 2,  // 最強ボス、クリア直前に1回だけ出現
	};

	class GameSceneFactory
	{
	public:
		static void CreateStateController();

		static void CreateBGM();

		static void CreateGround();

		///<summary>
		///見えない境界壁。プレイヤーがフィールド外へ出るのを防ぐ、描画されない静的コライダー
		///</summary>
		static void CreateFieldBoundary();

		///<summary>
		///Skybox。フィールド外側の何もない虚無が見えないようにする
		///</summary>
		static void CreateSkybox();

		static void CreatePlayer(const CreatePlayerContext& Context);

		static void CreateCamera();

		static void CreateDirLight();

		static void CreateStartEffect();

		///<summary>
		///体力バーのUIを生成する
		///</summary>
		static void CreateUI();

		///<summary>
		///武器を追加する。初期武器・パーク選択・デバッグ操作の全経路がこれを使う
		///</summary>
		///<returns>追加した武器エンティティ。空きスロットが無い場合はentt::null</returns>
		static entt::entity AddWeaponToPlayer(
			entt::entity player,
			eWeaponType type,
			int weaponId,
			eWeaponControl control);

		///<summary>
		///武器を削除する。デバッグ操作用で、SelfDefense等の子エンティティも合わせて破棄する
		///</summary>
		static void RemoveWeaponFromPlayer(entt::entity player, entt::entity weaponEntity);

	public:
		///<summary>
		///敵を生成する。EnemySpawnSystemから呼ばれる
		///</summary>
		///<param name="position">生成位置</param>
		///<param name="waveModifier">現在の難易度倍率、HPと攻撃力</param>
		///<param name="bossTier">eBossTier::None以外ならdata::BossDataの倍率を追加で掛けボスとして生成する</param>
		///<param name="enemyId">data::EnemyDataのId。ボスは常にId=0の強化版として生成する</param>
		static void CreateEnemy(
			const DirectX::XMFLOAT3& position,
			const ecs::EnemyWaveModifier& waveModifier,
			eBossTier bossTier,
			int enemyId = 0);

	private:
		static void CreatePlayerHpBar();

		///<summary>
		///必殺技ゲージを生成する。体力バーと同じ画像を左右反転・青色で右下に配置する
		///</summary>
		static void CreatePlayerUltimateGauge();

		///<summary>
		///所持武器アイコンバーを生成する。体力バーと必殺ゲージの間に横5×縦2で配置する
		///</summary>
		static void CreateWeaponIconBar();

		static void CreateWaveTimerUI();

		///<summary>
		///経験値バーと現在レベル表示UIを画面上部に生成する
		///</summary>
		static void CreatePlayerExpBar();

		///<summary>
		///PlayerSaveDataの恒久強化レベルをBaseへ反映する
		///</summary>
		static void ApplyStatUpgrades(ecs::PlayerStatusComponent& status);
	};
}


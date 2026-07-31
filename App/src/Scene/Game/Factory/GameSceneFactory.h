#pragma once

#include<cstdint>
#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Status/PlayerStatusComponent.h>

namespace ecs
{
	/// <summary>
	/// プレイヤー生成時に使用するコンテキスト
	/// </summary>
	struct CreatePlayerContext
	{
		uint32_t SelectSpellID;
	};

	/// <summary>
	/// ボースの階級。data::BossDataのIdと対応させる(int変換して使う)。
	/// Noneは通常の敵(ボースではない)を意味し、BossDataの参照は行わない。
	/// </summary>
	enum class eBossTier
	{
		None = -1,
		Mini = 0,   // 小ボース(周期的に複数回出現)
		Mid = 1,    // 中ボース(1回だけ出現)
		Final = 2,  // 最強ボース(クリア直前に1回だけ出現)
	};

	class GameSceneFactory
	{
	public:
		// 状態管理
		static void CreateStateController();

		// 背景音
		static void CreateBGM();

		// 地面
		static void CreateGround();

		// 見えない境界壁(プレイヤーがフィールド外へ出るのを防ぐ、描画されない静的コライダー)
		static void CreateFieldBoundary();

		// 空(Skybox)。フィールド外側の「何もない虚無」が見えないようにする
		static void CreateSkybox();

		// プレイヤー
		static void CreatePlayer(const CreatePlayerContext& Context);

		// カメラ
		static void CreateCamera();

		// ディレクションライト
		static void CreateDirLight();

		// 開始時のエフェクト生成
		static void CreateStartEffect();

		// ゲームクリア時のウィジェット

		// ゲームオーバー時の演出、ウィジェット

		// パーク選択ウィジェットとシステム

		// 体力バーのUI
		static void CreateUI();

		// 武器の追加（初期武器・パーク選択・デバッグ操作の全経路がこれを使う）
		// 戻り値: 追加した武器エンティティ（空きスロットが無い場合はentt::null）
		static entt::entity AddWeaponToPlayer(
			entt::entity player,
			eWeaponType type,
			int weaponId,
			eWeaponControl control);

		// 武器の削除（デバッグ操作用。SelfDefense等の子エンティティも合わせて破棄する）
		static void RemoveWeaponFromPlayer(entt::entity player, entt::entity weaponEntity);

	public:
		// 敵の生成（EnemySpawnSystemから呼ばれる）
		// position: 生成位置 / waveModifier: 現在の難易度倍率(HP・攻撃力)
		// bossTier: eBossTier::None以外ならdata::BossDataの倍率を追加で掛けてボースとして生成する
		// enemyId: data::EnemyDataのId(敵の種類。ボースは常にId=0の強化版として生成する)
		static void CreateEnemy(
			const DirectX::XMFLOAT3& position,
			const ecs::EnemyWaveModifier& waveModifier,
			eBossTier bossTier,
			int enemyId = 0);

	private:
		// プレイヤーの体力バー生成
		static void CreatePlayerHpBar();

		// 必殺技ゲージ生成（体力バーと同じ画像を左右反転・青色で右下に配置する）
		static void CreatePlayerUltimateGauge();

		// 所持武器アイコンバー生成（体力バーと必殺ゲージの間、横5×縦2で配置する）
		static void CreateWeaponIconBar();

		// 制限時間表示UIの生成
		static void CreateWaveTimerUI();

		// 経験値バー・現在レベル表示UIの生成(画面上部)
		static void CreatePlayerExpBar();

		// PlayerSaveData(ゴールドで購入したステータス恒久強化レベル)をBaseへ反映する
		static void ApplyStatUpgrades(ecs::PlayerStatusComponent& status);
	};
}


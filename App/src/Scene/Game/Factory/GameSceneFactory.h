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

	class GameSceneFactory
	{
	public:
		// 状態管理
		static void CreateStateController();

		// 背景音
		static void CreateBGM();

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
		// position: 生成位置 / waveModifier: 現在の難易度倍率(HP・攻撃力) / isBoss: ボースとして生成するか
		static void CreateEnemy(
			const DirectX::XMFLOAT3& position,
			const ecs::EnemyWaveModifier& waveModifier,
			bool isBoss);

	private:
		// プレイヤーの体力バー生成
		static void CreatePlayerHpBar();

		// 制限時間表示UIの生成
		static void CreateWaveTimerUI();

		// PlayerSaveData(ゴールドで購入したステータス恒久強化レベル)をBaseへ反映する
		static void ApplyStatUpgrades(ecs::PlayerStatusComponent& status);
	};
}


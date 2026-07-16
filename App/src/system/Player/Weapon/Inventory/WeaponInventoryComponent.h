#pragma once

#include<vector>
#include<entt/entt.hpp>

namespace ecs
{
	/// <summary>
	/// プレイヤーが所持する武器の管理をするコンポーネント。
    /// スロット上限を超えて新規武器を取得することはできない。
    /// （上限到達後はレベルアップのみ可能とする想定）
	/// </summary>
	struct WeaponInventoryComponent
	{
        /// <summary>所持している武器エンティティ（生成順）</summary>
        std::vector<entt::entity> Weapons;

        /// <summary>同時所持できる武器の最大数</summary>
        int MaxSlots = 6;

        /// <summary>現在の所持数</summary>
        int Count() const { return static_cast<int>(Weapons.size()); }

        /// <summary>新規武器を取得できるか（空きスロットがあるか）</summary>
        bool HasFreeSlot() const { return Count() < MaxSlots; }
	};

    /// <summary>
    /// 武器の攻撃種別。
    /// 取得するマスタ型（data::SingleShotWeaponData 等）の選択に使う。
    /// </summary>
    enum class eWeaponType
    {
        SingleShot, // 単発型：狙った方向へ弾を発射する
        SelfDefense,// 自衛型：プレイヤー周囲に持続的な当たり判定を張る
        AreaAttack, // 範囲攻撃：指定地点/自機周辺に範囲ダメージを発生させる
        Nova,       // 自己中心型：発動トリガーが無く、周期的にプレイヤー自身を中心とした範囲ダメージを発生させる
        Homing,     // 追尾型：狙い不要で自動的に近くの敵へ追尾弾を発射する
        Chain,      // 連鎖型：狙い不要で自動的に近くの敵を撃ち、命中した敵から別の敵へ跳ね移る
        Meteor,     // 隕石型：狙い不要で自動的に、周囲の敵複数体の頭上へ隕石を落とし範囲ダメージを与える
    };

    /// <summary>
    /// 武器の操作方式。
    /// </summary>
    enum class eWeaponControl
    {
        Manual, // 手動：入力（WantsToFire）が立ったフレームにのみ発射判定を行う
        Auto,   // 自動：CT が明けるたびにシステムが自走で発射する
    };


    /// <summary>
    /// 武器の共通情報の所持
    /// データ取得に必要な情報の保持
    /// </summary>
    struct WeaponComponent
    {
        // 武器種類ID
        int WeaponID = 0;

        // 攻撃種別
        eWeaponType Type = eWeaponType::SingleShot;

        // 最大レベル
        int MaxLevel = 0;

        // 現在のレベル
        int Level = 0;

        // アタッチ元のエンティティ
        entt::entity Owner = entt::null;

        // 操作方式
        eWeaponControl Control = eWeaponControl::Auto;

    };
}
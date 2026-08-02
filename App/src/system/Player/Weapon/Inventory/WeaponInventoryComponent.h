#pragma once

#include<vector>
#include<entt/entt.hpp>

namespace ecs
{
	///<summary>
	///プレイヤーが所持する武器の管理をするコンポーネント。スロット上限を超えて新規武器を取得することはできない、上限到達後はレベルアップのみ可能という想定
	///</summary>
	struct WeaponInventoryComponent
	{
        ///<summary>
        ///所持している武器エンティティ、生成順
        ///</summary>
        std::vector<entt::entity> Weapons;

        ///<summary>
        ///同時所持できる武器の最大数
        ///</summary>
        int MaxSlots = 10;

        ///<summary>
        ///現在の所持数
        ///</summary>
        int Count() const { return static_cast<int>(Weapons.size()); }

        ///<summary>
        ///新規武器を取得できるか、空きスロットがあるか
        ///</summary>
        bool HasFreeSlot() const { return Count() < MaxSlots; }
	};

    ///<summary>
    ///武器の攻撃種別。取得するランタイム型はdata::SingleShotWeaponData等の選択に使う
    ///</summary>
    enum class eWeaponType
    {
        SingleShot, // 単発型、狙った方向へ弾を発射する
        SelfDefense,// 自衛型、プレイヤー周囲に継続的な当たり判定を張る
        AreaAttack, // 範囲攻撃、指定地点や自機周辺に範囲ダメージを発生させる
        Nova,       // 自己中心型、発動トリガーが無く周期的にプレイヤー自身を中心とした範囲ダメージを発生させる
        Homing,     // 追従型、狙い不要で自動的に近くの敵へ追従弾を発射する
        Chain,      // 連鎖型、狙い不要で自動的に近くの敵を撃ち、命中した敵から別の敵へ跳ね移る
        Meteor,     // 隕石型、狙い不要で自動的に周囲の敵複数体の頭上へ隕石を落とし範囲ダメージを与える
        VoidBeam,   // 貫通レーザー型、狙い不要で自動的に最も近い敵の方向へ直線状の範囲を貫通させる
        BoneSpear,  // 貫通弾型、狙い不要で自動的に最も近い敵へ直進する貫通弾を発生させる
        Cleave,     // 近接薙ぎ払い型、狙い不要で自動的に狙った方向の扇状範囲内の敵をなぎ払いノックバックさせる
        FlickerStrike, // ワープ連撃型、手動発動限定。パワーチャージを全消費し近くの敵へ次々ワープ攻撃する
        Ricochet,      // 反射増殖型、狙い不要で自動的に近くの敵へ球体の弾を発射する。命中すると増殖しながら跳ね返り、世代の上限まで繰り返す
    };

    ///<summary>
    ///武器の操作方式
    ///</summary>
    enum class eWeaponControl
    {
        Manual, // 手動、入力WantsToFireが立ったフレームにのみ発動判定を行う
        Auto,   // 自動、CTが明けるたびにシステムが自走で発動する
    };


    ///<summary>
    ///武器の共通情報の所持。データ取得に必要な情報を保持する
    ///</summary>
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

        // アタッチ先のエンティティ
        entt::entity Owner = entt::null;

        // 操作方式
        eWeaponControl Control = eWeaponControl::Auto;

    };
}

#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// SelfDefense(周回)型武器のマスタデータ（CSV/DB）。
    /// Id = (WeaponID + 1) * 1000 + Level。武器種類ごとにLv1〜MaxLevelの行を持ち、
    /// レベルアップ時は該当Idの行を直接取得する（Base+PerLevelの実行時計算は行わない）。
    /// CSV ヘッダー名は各フィールド名と完全一致すること。
    ///
    /// 装備した瞬間からOrbCount個のオーブがプレイヤーを中心にOrbitRadius(m)の円周上を
    /// OrbitSpeed(度/秒)で周回し続ける（発動トリガーの無い常時稼働の持続武器）。
    /// 各オーブはHitRadius(m)の当たり判定を持ち、敵と接触している間はHitInterval(秒)
    /// ごとにダメージを反復する（OrbitWeaponSystemが担当）。
    /// </summary>
    struct OrbitWeaponData
    {
        int         Id = 0;                  // (WeaponID+1)*1000+Level（主キー）
        std::string Name;                    // 表示・デバッグ用

        float       Damage = 4.0f;           // 1オーブ・1ヒットあたりのこのレベルでの火力

        int         OrbCount = 2;            // 周回するオーブの数

        float       OrbitRadius = 9.0f;      // プレイヤー中心からの周回半径(m)
        float       OrbitSpeed = 120.0f;     // 周回速度(度/秒)

        float       HitRadius = 3.0f;        // オーブ1個あたりの当たり判定半径(m)
        float       HitInterval = 0.5f;      // 同じ相手に密着し続けた場合の再ダメージ間隔(秒)

        float       HeightOffset = 30.0f;    // 周回するY座標 = Owner.Position.y + この値(m)。
                                              // デフォルトはプレイヤーの胸あたりの高さ
                                              // (プレイヤー/敵共通のColliderComponent半径・y=30を目安とする)

        std::string OrbEffectPath;           // オーブ自体の常時ループエフェクト(.efk)
        std::string HitEffectPath;           // 命中時に1回だけ再生するエフェクト(.efk)

        REFLECT_BEGIN(OrbitWeaponData, "orbit_weapons")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_INT(OrbCount)
            REFLECT_FIELD_FLOAT(OrbitRadius)
            REFLECT_FIELD_FLOAT(OrbitSpeed)
            REFLECT_FIELD_FLOAT(HitRadius)
            REFLECT_FIELD_FLOAT(HitInterval)
            REFLECT_FIELD_FLOAT(HeightOffset)
            REFLECT_FIELD_STR(OrbEffectPath)
            REFLECT_FIELD_STR(HitEffectPath)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::OrbitWeaponData);

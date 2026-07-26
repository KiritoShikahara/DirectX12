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
    /// 装備すると、ActiveDuration(秒)の間だけOrbCount個のオーブがプレイヤーを中心に
    /// OrbitRadius(m)の円周上をOrbitSpeed(度/秒)で周回し、その後CooldownDuration(秒)の間は
    /// オーブが消滅する……というサイクルを繰り返す(常時稼働ではなくオンオフを繰り返す
    /// 持続武器)。各オーブはHitRadius(m)の当たり判定を持ち、敵と接触している間はHitInterval(秒)
    /// ごとにダメージを反復し、命中した敵にSpeedMultiplier倍の減速をSlowDuration(秒)だけ
    /// 付与する（OrbitWeaponSystemが担当。減速の実体はEnemySlowStatusComponent）。
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

        float       ActiveDuration = 6.0f;   // オーブが出現している時間(秒)
        float       CooldownDuration = 3.0f; // オーブが消滅しているクールタイム(秒)

        float       SlowMultiplier = 0.6f;   // 命中した敵の移動速度倍率(1.0=減速なし、0.5=50%減速)
        float       SlowDuration = 1.5f;     // 減速効果の持続時間(秒、命中のたびに更新される)

        // エフェクト素材ID(';'区切りで複数指定可、data::EffectAssetData参照)。
        // ecs::effectutil::ResolveEffectIds()でパス文字列へ解決してから使うこと。
        std::string OrbEffectIds;           // オーブ自体の常時ループエフェクト
        std::string HitEffectIds;           // 命中時に1回だけ再生するエフェクト

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
            REFLECT_FIELD_FLOAT(ActiveDuration)
            REFLECT_FIELD_FLOAT(CooldownDuration)
            REFLECT_FIELD_FLOAT(SlowMultiplier)
            REFLECT_FIELD_FLOAT(SlowDuration)
            REFLECT_FIELD_STR(OrbEffectIds)
            REFLECT_FIELD_STR(HitEffectIds)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::OrbitWeaponData);

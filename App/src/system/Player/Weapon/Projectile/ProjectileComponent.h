#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<string>

namespace graphics { class FbxResource; }

namespace ecs
{
    /// <summary>
    /// 飛び道具の共通データ。武器種別を問わず全ての飛翔体で共用する
    /// （新しい遠距離武器を追加する際もこのコンポーネントをそのまま使い回せる）。
    /// 移動は RigidBodyComponent(Kinematic) + MoveVelocity で行い、
    /// 当たり判定は SensorTagComponent + ColliderComponent(Sphere) で行う。
    /// </summary>
    struct ProjectileComponent
    {
        /// <summary>進行方向（正規化済み、Yは常に0）</summary>
        DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };

        /// <summary>弾速 m/s</summary>
        float Speed = 20.0f;

        /// <summary>命中時に爆発範囲内の敵へ与えるダメージ</summary>
        float Damage = 5.0f;

        /// <summary>命中時に発生する爆発の実際の当たり判定半径(m)</summary>
        float ExplosionRadius = 1.5f;

        /// <summary>着弾エフェクトの見た目のサイズ計算にのみ使う半径(m)。
        /// ExplosionRadius(判定半径)とは別に持ち、判定半径を拡大してもエフェクトの
        /// 見た目のサイズは変えずに済むようにする</summary>
        float VisualRadius = 1.5f;

        /// <summary>着弾時に再生する爆発エフェクトのアセットパス（空なら再生しない）</summary>
        std::string ExplosionEffectPath;

        /// <summary>何にも当たらなかった場合、生成からこの秒数が経過すると自動的に消滅する</summary>
        float LifeTime = 3.0f;

        /// <summary>経過時間(秒)。ProjectileMovementSystem が加算する</summary>
        float ElapsedTime = 0.0f;

        /// <summary>発射元エンティティ（将来、自傷防止や与ダメージ元表示等に使う想定）</summary>
        entt::entity Owner = entt::null;

        // ── 追尾弾(Homing Missile)専用 ──────────────────────────
        // 通常の弾はIsHoming=falseのままで、以下のフィールドは一切参照されない
        // （HomingMissileSteeringSystemがIsHoming==trueの弾のみ処理する）。

        /// <summary>true の場合、Targetへ向けて毎フレームDirectionをTurnSpeedの範囲内で回転させる</summary>
        bool IsHoming = false;

        /// <summary>追尾対象。無効(死亡/破棄)になった場合はHomingMissileSteeringSystemがHomingSearchRadius内で再捕捉する</summary>
        entt::entity Target = entt::null;

        /// <summary>追尾時の最大旋回速度(度/秒)</summary>
        float TurnSpeed = 0.0f;

        /// <summary>Targetが無効な場合に再捕捉を試みる範囲(m)</summary>
        float HomingSearchRadius = 0.0f;

        // ── 貫通弾(Bone Spear)専用 ──────────────────────────────
        // 通常の弾はPierceCount=0のままで、命中時に即座に消滅する(既存の全武器と同じ挙動)。

        /// <summary>命中してもこの回数だけ消滅せずに貫通する。命中のたびに1ずつ減算し、
        /// 0未満になったタイミングで消滅する(ProjectileCollisionSystemが処理)</summary>
        int PierceCount = 0;

        /// <summary>trueの場合、着弾エフェクト(ExplosionEffectPath)の再生位置のYを
        /// 実際の着弾高さ(HeightOffset分浮いた位置)ではなく地面(Y=0)に固定する。
        /// 飛翔中は胸の高さを飛ぶ弾でも、爆発は地面で起きているように見せたい武器
        /// (FireBolt等)向け。ダメージ判定(ApplyExplosionDamage)は実際の着弾位置のまま変えない。
        /// falseがデフォルトで、既存の武器(HomingMissile/BoneSpear等)の挙動は変わらない</summary>
        bool ExplosionAtGroundLevel = false;

        // ── 反射増殖弾(Ricochet)専用 ──────────────────────────────
        // 通常の弾はMaxGeneration=0のままで、命中しても増殖せず即座に消滅する
        // (既存の全武器と同じ挙動。ProjectileCollisionSystemが処理する)。

        /// <summary>この弾が今何世代目か(0=初弾)。増殖時、子弾にはGeneration+1を渡す</summary>
        int Generation = 0;

        /// <summary>増殖できる世代の上限。Generation&gt;=MaxGenerationの弾は命中しても
        /// 増殖せず、通常の弾と同じく消滅する(無限増殖を防ぐ)</summary>
        int MaxGeneration = 0;

        /// <summary>命中時に増殖する子弾の数</summary>
        int SplitCount = 0;

        /// <summary>子弾の対象(命中した敵を除く)を探す範囲(m)</summary>
        float SplitSearchRadius = 0.0f;

        // ── 見た目をプリミティブメッシュにしたい弾専用(Ricochet等) ──────────
        // 通常の弾(nullptrのまま)はエフェクトのみで見た目を表現するため、以下は参照されない。

        /// <summary>非nullptrの場合、この弾の見た目としてFbxComponentで描画するメッシュ
        /// (例: PrimitiveResourceManager::Get().GetResource("Sphere"))。
        /// 増殖時、子弾にもそのままコピーする(生成側がプリミティブの種類を知らなくて済むようにするため)</summary>
        graphics::FbxResource* VisualMeshResource = nullptr;

        /// <summary>VisualMeshResourceの描画スケール(m)</summary>
        float VisualMeshScale = 1.0f;

        /// <summary>VisualMeshResourceの乗算カラー</summary>
        DirectX::XMFLOAT4 VisualMeshColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    };
}

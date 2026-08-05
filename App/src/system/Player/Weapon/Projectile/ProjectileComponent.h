#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<string>

namespace graphics { class FbxResource; }

namespace ecs
{
    ///<summary>
    ///飛び道具の共通データ。武器種別を問わず全ての飛翔体で共用する
    ///</summary>
    struct ProjectileComponent
    {
        ///<summary>
        ///進行方向、正規化済み、Yは常に0
        ///</summary>
        DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };

        ///<summary>
        ///弾速、m/s
        ///</summary>
        float Speed = 20.0f;

        ///<summary>
        ///命中時に爆発範囲内の敵へ与えるダメージ
        ///</summary>
        float Damage = 5.0f;

        ///<summary>
        ///命中時に発生する爆発の実際の当たり判定半径、m
        ///</summary>
        float ExplosionRadius = 1.5f;

        ///<summary>
        ///着弾エフェクトの見た目のサイズ計算にのみ使う半径。ExplosionRadiusとは別に持ち判定半径拡大の影響を受けない。
        ///増殖弾の子弾コライダーサイズにも使われるため(ProjectileCollisionSystem::SpawnSplitProjectiles参照)、
        ///着弾エフェクトだけを個別に拡大したい場合はこちらではなくHitEffectVisualRadiusを使うこと
        ///</summary>
        float VisualRadius = 1.5f;

        ///<summary>
        ///0以上の場合、着弾エフェクトのスケール計算をVisualRadiusの代わりにこちらの値で行う。
        ///負値(既定)なら従来通りVisualRadiusを使う。増殖弾の子弾サイズ(VisualRadius由来)には影響しない
        ///</summary>
        float HitEffectVisualRadius = -1.0f;

        ///<summary>
        ///着弾時に再生する爆発エフェクトのアセットパス、空なら再生しない
        ///</summary>
        std::string ExplosionEffectPath;

        ///<summary>
        ///何にも当たらなかった場合、生成からこの秒数が経過すると自動的に消滅する
        ///</summary>
        float LifeTime = 3.0f;

        ///<summary>
        ///経過時間、秒。ProjectileMovementSystemが加算する
        ///</summary>
        float ElapsedTime = 0.0f;

        ///<summary>
        ///発射元エンティティ、将来の自傷防止や与ダメージ元表示等に使う想定
        ///</summary>
        entt::entity Owner = entt::null;

        ///<summary>
        ///trueの場合、Targetへ向けて毎フレームDirectionをTurnSpeedの範囲内で回転させる。追尾弾専用、通常の弾はfalseのまま
        ///</summary>
        bool IsHoming = false;

        ///<summary>
        ///追尾対象。無効になった場合はHomingMissileSteeringSystemがHomingSearchRadius内で再捕捉する
        ///</summary>
        entt::entity Target = entt::null;

        ///<summary>
        ///追尾時の最大旋回速度、度/秒
        ///</summary>
        float TurnSpeed = 0.0f;

        ///<summary>
        ///Targetが無効な場合に再捕捉を試みる範囲、m
        ///</summary>
        float HomingSearchRadius = 0.0f;

        ///<summary>
        ///命中してもこの回数だけ消滅せずに貫通する。貫通弾専用、通常の弾は0のまま即座に消滅する
        ///</summary>
        int PierceCount = 0;

        ///<summary>
        ///trueの場合、着弾エフェクトの再生位置のYを地面に固定する。飛翔中は胸の高さを飛ぶ弾でも爆発は地面で起きているように見せたい武器向け
        ///</summary>
        bool ExplosionAtGroundLevel = false;

        ///<summary>
        ///この弾が今何世代目か、0が初弾。増殖時、子弾にはGeneration+1を渡す。反射増殖弾専用
        ///</summary>
        int Generation = 0;

        ///<summary>
        ///増殖できる世代の上限。Generationがこれ以上の弾は命中しても増殖せず通常の弾と同じく消滅する
        ///</summary>
        int MaxGeneration = 0;

        ///<summary>
        ///命中時に増殖する子弾の数
        ///</summary>
        int SplitCount = 0;

        ///<summary>
        ///子弾の対象を探す範囲、m。命中した敵は除く
        ///</summary>
        float SplitSearchRadius = 0.0f;

        ///<summary>
        ///非nullptrの場合、この弾の見た目としてFbxComponentで描画するメッシュ。増殖時は子弾にもそのままコピーする
        ///</summary>
        graphics::FbxResource* VisualMeshResource = nullptr;

        ///<summary>
        ///VisualMeshResourceの描画スケール、m
        ///</summary>
        float VisualMeshScale = 1.0f;

        ///<summary>
        ///VisualMeshResourceの乗算カラー
        ///</summary>
        DirectX::XMFLOAT4 VisualMeshColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    };
}

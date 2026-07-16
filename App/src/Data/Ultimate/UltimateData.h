#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// プレイヤーの必殺技(Ultimate)用バランス調整データ(CSV/DB)。
    /// 常にId=0の単一行のみを使う設定値。CSV ヘッダー名は各フィールド名と完全一致すること。
    /// GUIからの調整は`debug::UltimateDebugPanel`(DataInspector)で行える。
    ///
    /// RequiredKillCount体の敵を倒すとゲージが満タンになり("Ultimate"アクション＝Qキー/PadR1)で
    /// 発動可能になる。満タンの間はプレイヤーにAuraEffectPathのオーラ(ループ)を纏わせる。
    ///
    /// 発動すると、以下の流れで進行する(PlayerUltimateSystemが担当。時間ではなく状態で
    /// 遷移するため、上昇距離のズレによる着地時のめり込み等が起きない)。
    /// 1. カメラを即座にプレイヤー正面・低い位置(見上げる構図)へ固定する(移動は伴わない)
    /// 2. プレイヤーが正面方向(発動時に捕捉)へRiseSpeedでまっすぐ上昇し、RiseHeightに達したら停止する
    /// 3. 停止した位置でBeamEffectPathを再生し、その再生が終わるまで待つ
    ///    (MaxBeamDurationを超えたら安全装置として強制的に次へ進む)
    /// 4. ビーム終了と同時に、プレイヤー座標・カメラを発動前の状態へ瞬時に戻す(テレポート)
    /// 5. 元の座標でその場にいる敵全員へDamageを与え、ActivationEffectPathを再生する
    ///
    /// AuraEffectPath/BeamEffectPath/ActivationEffectPathは';'区切りで複数のエフェクトパスを
    /// 指定でき、指定した数だけ同時に組み合わせて再生される(ecs::effectutil参照)。
    /// </summary>
    struct UltimateData
    {
        int Id = 0;

        int   RequiredKillCount = 15; // ゲージが満タンになるまでに必要な撃破数

        float Damage = 999.0f; // 詠唱時にステージ上の敵全員へ与えるダメージ

        float RiseSpeed = 150.0f;      // 上昇速度 m/s
        float RiseHeight = 150.0f;     // この高さ(発動時のY座標からの相対値、m)まで上昇する
        float MaxBeamDuration = 5.0f;  // ビームエフェクトの再生終了を待つ最大時間(秒、安全装置)

        float CameraDistance = 50.0f;    // 発動時のプレイヤー正面方向へこの距離だけ離した位置にカメラを置く
        float CameraHeight = 15.0f;      // カメラの高さ(発動時のプレイヤーY座標からのオフセット。
                                          // 低めにして上昇するプレイヤーを見上げる構図にする)
        float CameraLookOffset = 40.0f;  // 注視点をプレイヤー座標からこの高さだけ上げる(顔の高さ目安)

        // ビームをプレイヤー座標そのままに再生すると、カメラの正面まっすぐ延びる形になり
        // 奥行きが見えず視認しづらい。カメラ方向へこの距離だけ手前にずらして再生する
        float BeamCameraOffset = 20.0f;

        float AuraScale = 5.0f;       // 満タン中に纏うオーラの見た目倍率
                                      // (プレイヤーのTransform.Scale=0.2の分を補正した見た目上の倍率)
        float BeamScale = 8.0f;       // 上昇後に再生するビームエフェクトの見た目倍率
        float ActivationScale = 8.0f; // 復帰後に再生する爆発エフェクトの見た目倍率

        // 爆発エフェクトをY=0(発動前のプレイヤー座標そのまま)で再生すると地面に少しめり込むため、
        // この高さだけ上げて再生する
        float ActivationHeightOffset = 5.0f;

        std::string AuraEffectPath;       // 満タン中、常時ループでプレイヤーに纏わせるオーラエフェクト
                                           // (';'区切りで複数指定可)
        std::string BeamEffectPath;       // 上昇完了位置で再生し、終了を待つビームエフェクト
                                           // (';'区切りで複数指定可)
        std::string ActivationEffectPath; // 復帰後、元の座標で1回だけ再生する爆発エフェクト
                                           // (';'区切りで複数指定可)

        REFLECT_BEGIN(UltimateData, "ultimate_data")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_INT(RequiredKillCount)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(RiseSpeed)
            REFLECT_FIELD_FLOAT(RiseHeight)
            REFLECT_FIELD_FLOAT(MaxBeamDuration)
            REFLECT_FIELD_FLOAT(CameraDistance)
            REFLECT_FIELD_FLOAT(CameraHeight)
            REFLECT_FIELD_FLOAT(CameraLookOffset)
            REFLECT_FIELD_FLOAT(BeamCameraOffset)
            REFLECT_FIELD_FLOAT(AuraScale)
            REFLECT_FIELD_FLOAT(BeamScale)
            REFLECT_FIELD_FLOAT(ActivationScale)
            REFLECT_FIELD_FLOAT(ActivationHeightOffset)
            REFLECT_FIELD_STR(AuraEffectPath)
            REFLECT_FIELD_STR(BeamEffectPath)
            REFLECT_FIELD_STR(ActivationEffectPath)
        REFLECT_END()
    };
}

REFLECT_REGISTER(data::UltimateData);

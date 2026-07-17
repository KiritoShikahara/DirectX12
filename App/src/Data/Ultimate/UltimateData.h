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
    /// 1. カメラを即座にプレイヤー背後・高い位置(見下ろす構図)へ固定する(移動は伴わない)
    /// 2. プレイヤーが正面方向(発動時に捕捉)へRiseSpeedでまっすぐ上昇し、RiseHeightに達したら停止する
    /// 3. 停止した位置でBeamEffectPath(pre)を再生し、その再生が終わるまで待つ
    ///    (MaxBeamDurationを超えたら安全装置として強制的に次へ進む)
    /// 4. 引き続き同じ位置でActivationEffectPath(main)を再生し、その再生が終わるまで待つ
    ///    (MaxMainDurationを超えたら安全装置として強制的に次へ進む)
    /// 5. メイン終了と同時に、プレイヤー座標・カメラを発動前の状態へ瞬時に戻し(テレポート)、
    ///    元の座標でその場にいる敵全員へDamageを与える
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
        float MaxBeamDuration = 5.0f;  // ビーム(pre)エフェクトの再生終了を待つ最大時間(秒、安全装置)
        float MaxMainDuration = 5.0f;  // メイン(main)エフェクトの再生終了を待つ最大時間(秒、安全装置)

        float CameraDistance = 80.0f;    // 発動時のプレイヤー背後方向(-forward)へこの距離だけ離した位置にカメラを置く
        float CameraHeight = 700.0f;     // カメラの高さ(発動時のプレイヤーY座標からのオフセット。
                                          // RiseHeightより高くして、上昇中ずっと見下ろす構図を維持する)
        float CameraLookOffset = 20.0f;  // 注視点をプレイヤー座標からこの高さだけ上げる(頭上あたりの目安)

        // ビームはプレイヤーの真下(地面方向)を向けて再生する。プレイヤー座標そのままだと
        // 自機モデルの足元と重なって見えるため、この距離だけ下にずらした位置から再生する
        float BeamDownOffset = 20.0f;

        float AuraScale = 5.0f;       // 満タン中に纏うオーラの見た目倍率
                                      // (プレイヤーのTransform.Scale=0.2の分を補正した見た目上の倍率)
        float BeamScale = 8.0f;       // ビーム(pre)エフェクトの見た目倍率
        float ActivationScale = 8.0f; // メイン(main)エフェクトの見た目倍率

        // メイン(main)エフェクトはビーム(pre)と同じ上昇後の座標(まだ地面へ戻す前)で再生する。
        // プレイヤーの中心からこの高さだけ上げて再生する(自機モデルに埋もれないようにする目安)
        float ActivationHeightOffset = 5.0f;

        std::string AuraEffectPath;       // 満タン中、常時ループでプレイヤーに纏わせるオーラエフェクト
                                           // (';'区切りで複数指定可)
        std::string BeamEffectPath;       // 上昇完了位置で再生し、終了を待つビーム(pre)エフェクト
                                           // (';'区切りで複数指定可)
        std::string ActivationEffectPath; // ビーム(pre)終了後、同じ位置で再生し終了を待つ
                                           // メイン(main)エフェクト。これの再生終了後に座標を
                                           // 元へ戻しダメージを与える(';'区切りで複数指定可)

        REFLECT_BEGIN(UltimateData, "ultimate_data")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_INT(RequiredKillCount)
            REFLECT_FIELD_FLOAT(Damage)
            REFLECT_FIELD_FLOAT(RiseSpeed)
            REFLECT_FIELD_FLOAT(RiseHeight)
            REFLECT_FIELD_FLOAT(MaxBeamDuration)
            REFLECT_FIELD_FLOAT(MaxMainDuration)
            REFLECT_FIELD_FLOAT(CameraDistance)
            REFLECT_FIELD_FLOAT(CameraHeight)
            REFLECT_FIELD_FLOAT(CameraLookOffset)
            REFLECT_FIELD_FLOAT(BeamDownOffset)
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

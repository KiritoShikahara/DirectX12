#pragma once

#include<DirectXMath.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
    /// <summary>必殺技発動中(IsActive)の内部フェーズ</summary>
    enum class eUltimatePhase
    {
        Ascending,   // 指定の高さ(RiseHeight)まで上昇中
        PlayingBeam, // 上昇完了、ビーム(pre)エフェクトの再生終了を待っている
        PlayingMain, // ビーム(pre)終了、メイン(main)エフェクトの再生終了を待っている
    };

    /// <summary>
    /// プレイヤーの必殺技(Ultimate)ゲージ・発動状態を保持するランタイムコンポーネント。
    /// 状態遷移・演出はPlayerUltimateSystemが担当する。
    /// </summary>
    struct PlayerUltimateComponent
    {
        /// <summary>現在の撃破数（UltimateData::RequiredKillCountに達するとIsReady=trueになる）</summary>
        int KillCount = 0;

        /// <summary>ゲージが満タンで発動可能か</summary>
        bool IsReady = false;

        /// <summary>発動演出(上昇・ビーム再生待ち)の最中か</summary>
        bool IsActive = false;

        /// <summary>IsActive中の内部フェーズ</summary>
        eUltimatePhase Phase = eUltimatePhase::Ascending;

        /// <summary>
        /// 現在のフェーズ(PlayingBeam/PlayingMain)に入ってからの経過時間(秒、rawDeltaTime基準)。
        /// フェーズが切り替わるたびに0にリセットする。UltimateData::MaxBeamDuration/
        /// MaxMainDurationに達したら、再生中でも強制的に次へ進める安全装置に使う
        /// (エフェクトパス未設定やアセット異常でIsPlaying()が永久にtrueのままになるケースに備える)。
        /// </summary>
        float PhaseElapsedTime = 0.0f;

        /// <summary>発動した瞬間のプレイヤー座標（上昇前の地面位置。カメラ位置・詠唱エフェクト・
        /// 復帰時のテレポート先の基準にする）</summary>
        DirectX::XMFLOAT3 StartPosition = { 0.0f, 0.0f, 0.0f };

        /// <summary>発動した瞬間のプレイヤーの正面方向（カメラをこの向きの前方へ配置する基準にする）</summary>
        DirectX::XMFLOAT3 ForwardDir = { 0.0f, 0.0f, 1.0f };

        /// <summary>満タン中にプレイヤーへ纏わせているオーラのエフェクトエンティティ一覧（複数対応）</summary>
        std::vector<entt::entity> AuraEffectEntities;

        /// <summary>上昇完了後に再生したビーム(pre)エフェクトのエンティティ一覧（再生終了監視用）</summary>
        std::vector<entt::entity> BeamEffectEntities;

        /// <summary>ビーム(pre)終了後に再生したメイン(main)エフェクトのエンティティ一覧（再生終了監視用）</summary>
        std::vector<entt::entity> MainEffectEntities;
    };

    /// <summary>
    /// プレイヤーが必殺技演出中(IsActive)かどうかを判定する。
    /// 演出中は他の武器の発動・既存の弾/オーブの挙動を一時停止させるため、各武器Systemの
    /// Update()冒頭から参照する（GameStateComponentの"InGame中のみ"チェックと同じ使い方）。
    /// </summary>
    inline bool IsPlayerUltimateActive(entt::registry& registry)
    {
        bool isActive = false;
        registry.view<PlayerUltimateComponent>().each(
            [&](const PlayerUltimateComponent& ultimate)
            {
                if (ultimate.IsActive) isActive = true;
            });
        return isActive;
    }
}

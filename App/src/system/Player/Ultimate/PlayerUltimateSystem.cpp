#include "apppch.h"
#include "PlayerUltimateSystem.h"

#include"PlayerUltimateComponent.h"
#include<system/Player/Status/PlayerStatusComponent.h>
#include<system/Player/Status/PlayerCombatUtil.h>
#include<system/Enemy/Status/EnemyStatusComponent.h>
#include<Data/Ultimate/UltimateData.h>
#include<Scene/Game/State/GameState.h>
#include<Tag/EntityTag.h>
#include<system/Effect/EffectSpawnUtility.h>

#include<system/Camera/CameraSystem.h>
#include<system/CameraFollow/CameraOverrideComponent.h>

#include<cmath>
#include<algorithm>

namespace
{
    /// <summary>
    /// ビーム素材は既定でローカル+Z(Effekseer Editorの青軸方向)を向いている前提で、
    /// その先端が正規化済みdirの方向を向くようなオイラー角(ピッチ=X軸回転、ヨー=Y軸回転)を
    /// 算出する。
    /// </summary>
    DirectX::XMFLOAT3 ComputeBeamRotationFromDirection(const DirectX::XMFLOAT3& dir)
    {
        const float pitch = std::asin(std::clamp(-dir.y, -1.0f, 1.0f));
        const float yaw = std::atan2(dir.x, dir.z);
        return { pitch, yaw, 0.0f };
    }

    /// <summary>
    /// 必殺技演出中、他の武器のエフェクトを隠す(非表示化と同時にEffekseer側も一時停止する。
    /// EffectComponent::IsVisible参照)。演出開始時にfalseで呼び、ビーム(hougu_pre)の
    /// 再生終了時にtrueで呼んで元に戻す。武器Systemの発動自体はIsPlayerUltimateActive()で
    /// 別途停止しているため、この時間帯に新規のエフェクトが増えることはない。
    /// </summary>
    void SetOtherEffectsVisible(entt::registry& registry, bool visible)
    {
        registry.view<ecs::EffectComponent>().each(
            [&](ecs::EffectComponent& effect)
            {
                effect.IsVisible = visible;
            });
    }
}

namespace ecs
{
    void PlayerUltimateSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
    {
        // InGame中のみ動作する（PerkSelect/Result中は発動できないようにする）
        auto stateView = registry.view<::ecs::GameStateComponent>();
        if (stateView.begin() == stateView.end()) return;
        if (registry.get<::ecs::GameStateComponent>(*stateView.begin()).GameState != ::sys::eGameState::InGame) return;

        auto playerView = registry.view<ecs::PlayerTag, ecs::PlayerUltimateComponent, ecs::PlayerStatusComponent>();
        if (playerView.begin() == playerView.end()) return;

        const entt::entity playerEntity = *playerView.begin();
        auto& ultimate = registry.get<ecs::PlayerUltimateComponent>(playerEntity);
        auto& status = registry.get<ecs::PlayerStatusComponent>(playerEntity);

        const auto* masterData = DATA_MGR(data::UltimateData).GetById(0);
        if (masterData == nullptr) return;

        if (ultimate.IsActive)
        {
            UpdateActive(registry, playerEntity, ultimate, status, *masterData, rawDeltaTime);
            return;
        }

        // ゲージが閾値に到達したら満タンにし、オーラを纏わせる
        if (!ultimate.IsReady && ultimate.KillCount >= masterData->RequiredKillCount)
        {
            ultimate.IsReady = true;
            StartAura(registry, playerEntity, ultimate, *masterData);
        }

        if (!ultimate.IsReady) return;

        if (::sys::InputManager::Get().IsActionPressed("Ultimate"))
        {
            Activate(registry, playerEntity, ultimate, status, *masterData);
        }
    }

    /// <summary>ゲージが満タンになった瞬間、オーラ(ループ、複数対応)エフェクトをプレイヤーへ付与する</summary>
    void PlayerUltimateSystem::StartAura(
        entt::registry& registry,
        entt::entity playerEntity,
        ecs::PlayerUltimateComponent& ultimate,
        const data::UltimateData& masterData)
    {
        if (!ultimate.AuraEffectEntities.empty()) return; // 既に付与済み

        ecs::effectutil::PlayLoopingCombined(
            masterData.AuraEffectPath, playerEntity, masterData.AuraScale, ultimate.AuraEffectEntities);
    }

    /// <summary>必殺技を発動する：オーラ解除、無敵化、カメラを即座に固定、上昇開始</summary>
    void PlayerUltimateSystem::Activate(
        entt::registry& registry,
        entt::entity playerEntity,
        ecs::PlayerUltimateComponent& ultimate,
        ecs::PlayerStatusComponent& status,
        const data::UltimateData& masterData)
    {
        const auto* playerTransform = registry.try_get<ecs::Transform>(playerEntity);
        if (playerTransform == nullptr) return;

        ultimate.IsActive = true;
        ultimate.IsReady = false;
        ultimate.Phase = ecs::eUltimatePhase::Ascending;
        ultimate.PhaseElapsedTime = 0.0f;
        ultimate.KillCount = 0;
        ultimate.StartPosition = playerTransform->GetPosition();
        DirectX::XMStoreFloat3(&ultimate.ForwardDir, playerTransform->GetForward());

        // オーラ(複数対応)を解除する（上昇演出へ切り替えるため）
        for (entt::entity auraEntity : ultimate.AuraEffectEntities)
        {
            if (registry.valid(auraEntity)) registry.destroy(auraEntity);
        }
        ultimate.AuraEffectEntities.clear();

        // 無敵化
        status.IsInvincible = true;

        // 他の武器のエフェクトを非表示化する（武器の発動自体はIsPlayerUltimateActive()で
        // 各武器Systemが停止するため、この時点で存在するエフェクトのみが対象になる）
        SetOtherEffectsVisible(registry, false);

        // 1. カメラを即座にプレイヤー正面・低い位置(見上げる構図)へ固定する
        UpdateCamera(registry, playerEntity, ultimate, masterData);

        // 2. 上昇開始：RigidBodyの速度で駆動する（Dynamic Bodyは物理側が位置の権威のため、
        // Transformを直接書き換えるのではなくMoveVelocity経由で移動させる。
        // PlayerMovementSystemのX/Z制御と同じ方式）
        auto* rigid = registry.try_get<ecs::RigidBodyComponent>(playerEntity);
        if (rigid != nullptr)
        {
            rigid->MoveVelocity = { 0.0f, masterData.RiseSpeed, 0.0f };
            rigid->HasMoveRequest = true;
        }
    }

    /// <summary>発動中(IsActive)の毎フレーム処理：Ascending/PlayingBeam/PlayingMainフェーズの遷移判定</summary>
    void PlayerUltimateSystem::UpdateActive(
        entt::registry& registry,
        entt::entity playerEntity,
        ecs::PlayerUltimateComponent& ultimate,
        ecs::PlayerStatusComponent& status,
        const data::UltimateData& masterData,
        float rawDeltaTime)
    {
        // カメラは発動中ずっと固定位置・見下ろすLookAtのまま(毎フレーム上書きし続ける)
        UpdateCamera(registry, playerEntity, ultimate, masterData);

        const auto* playerTransform = registry.try_get<ecs::Transform>(playerEntity);

        if (ultimate.Phase == ecs::eUltimatePhase::Ascending)
        {
            const float currentHeight = playerTransform != nullptr
                ? playerTransform->GetPosition().y - ultimate.StartPosition.y
                : masterData.RiseHeight; // Transform取得失敗時は無限待機を避けるため即座に完了扱いにする

            if (currentHeight < masterData.RiseHeight) return; // 上昇継続中

            // 3. 上昇完了：静止し、ビーム(pre)エフェクトを再生してその終了を待つフェーズへ
            auto* rigid = registry.try_get<ecs::RigidBodyComponent>(playerEntity);
            if (rigid != nullptr)
            {
                rigid->MoveVelocity = { 0.0f, 0.0f, 0.0f };
                rigid->HasMoveRequest = true;
            }

            ultimate.Phase = ecs::eUltimatePhase::PlayingBeam;
            ultimate.PhaseElapsedTime = 0.0f;

            const DirectX::XMFLOAT3 beamPosition = playerTransform != nullptr
                ? playerTransform->GetPosition()
                : ultimate.StartPosition;

            // ビームの先端が地面(真下)を向くよう固定方向で再生する。プレイヤー座標そのままだと
            // 自機モデルの足元と重なって見えるため、BeamDownOffset分だけ下にずらして再生する
            const DirectX::XMFLOAT3 beamRotation = ComputeBeamRotationFromDirection({ 0.0f, -1.0f, 0.0f });
            const DirectX::XMFLOAT3 beamSpawnPosition =
            {
                beamPosition.x,
                beamPosition.y - masterData.BeamDownOffset,
                beamPosition.z,
            };

            ecs::effectutil::PlayOneShotCombined(
                masterData.BeamEffectPath, beamSpawnPosition, masterData.BeamScale,
                &ultimate.BeamEffectEntities, beamRotation);
            return;
        }

        if (ultimate.Phase == ecs::eUltimatePhase::PlayingBeam)
        {
            // ビーム(pre)の再生終了(またはMaxBeamDurationでのタイムアウト)を待つ
            ultimate.PhaseElapsedTime += rawDeltaTime;
            const bool beamStillPlaying = ecs::effectutil::AnyPlaying(registry, ultimate.BeamEffectEntities);
            const bool beamTimedOut = ultimate.PhaseElapsedTime >= masterData.MaxBeamDuration;

            if (beamStillPlaying && !beamTimedOut) return;

            // 4. ビーム終了：まだ座標は戻さず、同じ位置でメイン(main)エフェクトを再生してその終了を待つ
            ultimate.Phase = ecs::eUltimatePhase::PlayingMain;
            ultimate.PhaseElapsedTime = 0.0f;

            const DirectX::XMFLOAT3 mainPosition = playerTransform != nullptr
                ? playerTransform->GetPosition()
                : ultimate.StartPosition;
            const DirectX::XMFLOAT3 mainSpawnPosition =
            {
                mainPosition.x,
                mainPosition.y + masterData.ActivationHeightOffset,
                mainPosition.z,
            };

            ecs::effectutil::PlayOneShotCombined(
                masterData.ActivationEffectPath, mainSpawnPosition, masterData.ActivationScale,
                &ultimate.MainEffectEntities);
            return;
        }

        // PlayingMainフェーズ：メイン(main)の再生終了(またはMaxMainDurationでのタイムアウト)を待つ
        ultimate.PhaseElapsedTime += rawDeltaTime;
        const bool mainStillPlaying = ecs::effectutil::AnyPlaying(registry, ultimate.MainEffectEntities);
        const bool mainTimedOut = ultimate.PhaseElapsedTime >= masterData.MaxMainDuration;

        if (mainStillPlaying && !mainTimedOut) return;

        FinishAndExplode(registry, playerEntity, ultimate, status, masterData);
    }

    /// <summary>
    /// プレイヤー背後・高い位置から見下ろす構図になるよう、カメラエンティティへ
    /// CameraOverrideComponentでリクエストを発行する(実際のTransform書き込みは
    /// CameraPlayerFollowSystemが一元的に行う。カメラのTransformを直接書き換えないことで、
    /// カメラ制御の責務をCameraPlayerFollowSystemへ集約している)。
    /// </summary>
    void PlayerUltimateSystem::UpdateCamera(
        entt::registry& registry,
        entt::entity playerEntity,
        const ecs::PlayerUltimateComponent& ultimate,
        const data::UltimateData& masterData)
    {
        const entt::entity cameraEntity = ::sys::CameraSystem::Get().GetMainCameraEntity();
        if (cameraEntity == entt::null || !registry.valid(cameraEntity)) return;

        const auto* playerTransform = registry.try_get<ecs::Transform>(playerEntity);
        if (playerTransform == nullptr) return;

        const DirectX::XMFLOAT3& start = ultimate.StartPosition;
        const DirectX::XMFLOAT3& forward = ultimate.ForwardDir;

        auto& cameraOverride = registry.get_or_emplace<ecs::CameraOverrideComponent>(cameraEntity);

        // 発動時に捕捉したプレイヤーの背後方向(-forward)へCameraDistance離れた、高い位置
        // (CameraHeight)から見下ろす構図にする。カメラ自体の位置は発動中ずっと固定で、
        // 追従はしない(LookAtだけが現在のプレイヤー座標へ追従する)。
        cameraOverride.Position =
        {
            start.x - forward.x * masterData.CameraDistance,
            start.y + masterData.CameraHeight,
            start.z - forward.z * masterData.CameraDistance,
        };

        const DirectX::XMFLOAT3& playerPos = playerTransform->GetPosition();
        cameraOverride.LookAt =
        {
            playerPos.x,
            playerPos.y + masterData.CameraLookOffset,
            playerPos.z,
        };
    }

    /// <summary>メイン(main)終了後：プレイヤー座標・無敵状態を戻し、その場で全体ダメージを与える</summary>
    void PlayerUltimateSystem::FinishAndExplode(
        entt::registry& registry,
        entt::entity playerEntity,
        ecs::PlayerUltimateComponent& ultimate,
        ecs::PlayerStatusComponent& status,
        const data::UltimateData& masterData)
    {
        // メイン(hougu_main)の再生が終わったので、非表示化していた他の武器のエフェクトを
        // 元に戻す（これ以降、各武器Systemの発動もIsPlayerUltimateActive()=falseになり再開する）
        SetOtherEffectsVisible(registry, true);

        // カメラのリクエストを取り下げ、CameraPlayerFollowSystemの通常追従へ戻す
        const entt::entity cameraEntity = ::sys::CameraSystem::Get().GetMainCameraEntity();
        if (cameraEntity != entt::null && registry.valid(cameraEntity))
        {
            registry.remove<ecs::CameraOverrideComponent>(cameraEntity);
        }

        // 5. プレイヤーの座標を瞬時に発動前の位置へ戻す(テレポート)。
        // Dynamic Bodyは物理側が位置の権威のため、Transformを直接書き換えただけでは
        // 次の物理ステップでJolt側の位置により上書きされてしまう。TransformDirtyTagを
        // 付与するとPhysicsSystem::SyncFromTransformがこのTransformの値をJolt側へ
        // 明示的に反映してくれるため、これを使って正確にテレポートする
        // (以前、速度ベースの降下で戻していた際に着地位置がズレて地面へめり込む不具合があったため、
        // このテレポート方式に変更した)。
        auto* playerTransform = registry.try_get<ecs::Transform>(playerEntity);
        if (playerTransform != nullptr)
        {
            playerTransform->SetPosition(ultimate.StartPosition);
            registry.emplace_or_replace<ecs::TransformDirtyTag>(playerEntity);
        }

        auto* rigid = registry.try_get<ecs::RigidBodyComponent>(playerEntity);
        if (rigid != nullptr)
        {
            rigid->MoveVelocity = { 0.0f, 0.0f, 0.0f };
            rigid->HasMoveRequest = true;
        }

        status.IsInvincible = false;

        // その場(元の座標)にいる敵全員へ大ダメージ(メイン(main)エフェクトは既にPlayingMainフェーズで
        // 再生済みのため、ここでは座標復元とダメージ適用のみ行う)。
        // DamagedByUltimate=trueにしておくことで、この後EnemyDeathSystemが処理する撃破が
        // 必殺技ゲージへ加算されないようにする(発動直後に即ゲージが貯まる自己参照を防ぐため。
        // ゴールド・経験値・パワーチャージは通常どおり加算される)
        registry.view<ecs::EnemyTag, ecs::EnemyStatusComponent, ecs::Transform>().each(
            [&](ecs::EnemyStatusComponent& enemyStatus, ecs::Transform& enemyTransform)
            {
                enemyStatus.CurrentHp = std::max(0.0f, enemyStatus.CurrentHp - masterData.Damage);
                enemyStatus.DamagedByUltimate = true;
                ecs::combatutil::SpawnDamageNumber(enemyTransform.GetPosition(), masterData.Damage, false);
            });

        ultimate.IsActive = false;
        ultimate.BeamEffectEntities.clear();
        ultimate.MainEffectEntities.clear();
    }
}

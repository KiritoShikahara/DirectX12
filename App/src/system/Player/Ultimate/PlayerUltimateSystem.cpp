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
    DirectX::XMFLOAT3 ComputeBeamRotationFromDirection(const DirectX::XMFLOAT3& dir)
    {
        const float pitch = std::asin(std::clamp(-dir.y, -1.0f, 1.0f));
        const float yaw = std::atan2(dir.x, dir.z);
        return { pitch, yaw, 0.0f };
    }

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
        // InGame中のみ動作する、PerkSelect/Result中は発動できない
        auto stateView = registry.view<::ecs::GameStateComponent>();
        if (stateView.begin() == stateView.end()) return;
        if (registry.get<::ecs::GameStateComponent>(*stateView.begin()).GameState != ::sys::eGameState::InGame) return;
        // 設定メニュー表示中は必殺技も発動できないようにする
        if (::ecs::IsOptionsMenuOpen(registry)) return;

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

        // オーラを解除する、上昇演出へ切り替えるため
        for (entt::entity auraEntity : ultimate.AuraEffectEntities)
        {
            if (registry.valid(auraEntity)) registry.destroy(auraEntity);
        }
        ultimate.AuraEffectEntities.clear();

        // 無敵化
        status.IsInvincible = true;

        // 他の武器のエフェクトを非表示化する。武器の発動自体は各武器Systemが停止するため、この時点で存在するエフェクトのみが対象になる
        SetOtherEffectsVisible(registry, false);

        // 1. カメラを即座にプレイヤー正面・低い位置へ固定する、見上げる構図
        UpdateCamera(registry, playerEntity, ultimate, masterData);

        // 2. 上昇開始、RigidBodyの速度で駆動する。Dynamic Bodyは物理側が位置の権威のためMoveVelocity経由で移動させる
        auto* rigid = registry.try_get<ecs::RigidBodyComponent>(playerEntity);
        if (rigid != nullptr)
        {
            rigid->MoveVelocity = { 0.0f, masterData.RiseSpeed, 0.0f };
            rigid->HasMoveRequest = true;
        }
    }

    void PlayerUltimateSystem::UpdateActive(
        entt::registry& registry,
        entt::entity playerEntity,
        ecs::PlayerUltimateComponent& ultimate,
        ecs::PlayerStatusComponent& status,
        const data::UltimateData& masterData,
        float rawDeltaTime)
    {
        // カメラは発動中ずっと固定位置・見下ろすLookAtのまま、毎フレーム上書きし続ける
        UpdateCamera(registry, playerEntity, ultimate, masterData);

        const auto* playerTransform = registry.try_get<ecs::Transform>(playerEntity);

        if (ultimate.Phase == ecs::eUltimatePhase::Ascending)
        {
            const float currentHeight = playerTransform != nullptr
                ? playerTransform->GetPosition().y - ultimate.StartPosition.y
                : masterData.RiseHeight; // Transform取得失敗時は無限待機を避けるため即座に完了扱いにする

            if (currentHeight < masterData.RiseHeight) return; // 上昇継続中

            // 3. 上昇完了、静止しビームエフェクトを再生してその終了を待つフェーズへ
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

            // ビームの先端が地面を向くよう固定方向で再生する。プレイヤー座標のままだと自機モデルの足元と重なるためBeamDownOffset分だけ下にずらす
            const DirectX::XMFLOAT3 beamRotation = ComputeBeamRotationFromDirection({ 0.0f, -1.0f, 0.0f });
            const DirectX::XMFLOAT3 beamSpawnPosition =
            {
                beamPosition.x,
                beamPosition.y - masterData.BeamDownOffset,
                beamPosition.z,
            };

            // ビームは演出進行に必須のため負荷間引きを無視して必ず生成する。間引かれると即座に次フェーズへ飛んで演出が破綻する
            ecs::effectutil::PlayOneShotCombined(
                masterData.BeamEffectPath, beamSpawnPosition, masterData.BeamScale,
                &ultimate.BeamEffectEntities, beamRotation, true);
            return;
        }

        if (ultimate.Phase == ecs::eUltimatePhase::PlayingBeam)
        {
            // ビームの再生終了、またはMaxBeamDurationでのタイムアウトを待つ
            ultimate.PhaseElapsedTime += rawDeltaTime;
            const bool beamStillPlaying = ecs::effectutil::AnyPlaying(registry, ultimate.BeamEffectEntities);
            const bool beamTimedOut = ultimate.PhaseElapsedTime >= masterData.MaxBeamDuration;

            if (beamStillPlaying && !beamTimedOut) return;

            // 4. ビーム終了、座標は戻さず同じ位置でメインエフェクトを再生してその終了を待つ
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

            // メインも演出の必須要素のため間引きを無視して必ず生成する。rotationは既定のまま
            ecs::effectutil::PlayOneShotCombined(
                masterData.ActivationEffectPath, mainSpawnPosition, masterData.ActivationScale,
                &ultimate.MainEffectEntities, { 0.0f, 0.0f, 0.0f }, true);
            return;
        }

        // PlayingMainフェーズ、メインの再生終了またはMaxMainDurationでのタイムアウトを待つ
        ultimate.PhaseElapsedTime += rawDeltaTime;
        const bool mainStillPlaying = ecs::effectutil::AnyPlaying(registry, ultimate.MainEffectEntities);
        const bool mainTimedOut = ultimate.PhaseElapsedTime >= masterData.MaxMainDuration;

        if (mainStillPlaying && !mainTimedOut) return;

        FinishAndExplode(registry, playerEntity, ultimate, status, masterData);
    }

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

        // 発動時に捕捉したプレイヤーの背後方向へCameraDistance離れた高い位置から見下ろす構図にする。カメラ位置は発動中ずっと固定でLookAtだけが追従する
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

    void PlayerUltimateSystem::FinishAndExplode(
        entt::registry& registry,
        entt::entity playerEntity,
        ecs::PlayerUltimateComponent& ultimate,
        ecs::PlayerStatusComponent& status,
        const data::UltimateData& masterData)
    {
        // メインの再生が終わったので、非表示化していた他の武器のエフェクトを元に戻す
        SetOtherEffectsVisible(registry, true);

        // カメラのリクエストを取り下げ、CameraPlayerFollowSystemの通常追従へ戻す
        const entt::entity cameraEntity = ::sys::CameraSystem::Get().GetMainCameraEntity();
        if (cameraEntity != entt::null && registry.valid(cameraEntity))
        {
            registry.remove<ecs::CameraOverrideComponent>(cameraEntity);
        }

        // 5. プレイヤー座標を瞬時に発動前の位置へ戻す。TransformDirtyTagを付与しPhysicsSystem::SyncFromTransformでJolt側へも反映させる
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

        // その場にいる敵全員へ大ダメージ。DamagedByUltimate=trueにして、この撃破が必殺技ゲージへ加算されないようにする
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

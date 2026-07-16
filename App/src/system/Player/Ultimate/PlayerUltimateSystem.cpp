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

#include<cmath>
#include<algorithm>

namespace
{
    /// <summary>ゼロベクトルなら{0,0,0}を返す安全な正規化</summary>
    DirectX::XMFLOAT3 NormalizeOrZero(const DirectX::XMFLOAT3& v)
    {
        const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
        if (lenSq < 0.0001f) return { 0.0f, 0.0f, 0.0f };

        const float invLen = 1.0f / std::sqrt(lenSq);
        return { v.x * invLen, v.y * invLen, v.z * invLen };
    }

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
        ultimate.BeamElapsedTime = 0.0f;
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

    /// <summary>発動中(IsActive)の毎フレーム処理：Ascending/PlayingBeamフェーズの遷移判定</summary>
    void PlayerUltimateSystem::UpdateActive(
        entt::registry& registry,
        entt::entity playerEntity,
        ecs::PlayerUltimateComponent& ultimate,
        ecs::PlayerStatusComponent& status,
        const data::UltimateData& masterData,
        float rawDeltaTime)
    {
        // カメラは発動中ずっと固定位置・見上げるLookAtのまま(毎フレーム上書きし続ける)
        UpdateCamera(registry, playerEntity, ultimate, masterData);

        const auto* playerTransform = registry.try_get<ecs::Transform>(playerEntity);

        if (ultimate.Phase == ecs::eUltimatePhase::Ascending)
        {
            const float currentHeight = playerTransform != nullptr
                ? playerTransform->GetPosition().y - ultimate.StartPosition.y
                : masterData.RiseHeight; // Transform取得失敗時は無限待機を避けるため即座に完了扱いにする

            if (currentHeight < masterData.RiseHeight) return; // 上昇継続中

            // 3. 上昇完了：静止し、ビームエフェクトを再生してその終了を待つフェーズへ
            auto* rigid = registry.try_get<ecs::RigidBodyComponent>(playerEntity);
            if (rigid != nullptr)
            {
                rigid->MoveVelocity = { 0.0f, 0.0f, 0.0f };
                rigid->HasMoveRequest = true;
            }

            ultimate.Phase = ecs::eUltimatePhase::PlayingBeam;
            ultimate.BeamElapsedTime = 0.0f;

            const DirectX::XMFLOAT3 beamPosition = playerTransform != nullptr
                ? playerTransform->GetPosition()
                : ultimate.StartPosition;

            // ビームの先端がカメラの方向を向くよう、現在のカメラ座標(UpdateCameraで
            // このフレーム分は更新済み)から回転を算出する。また、プレイヤー座標そのままだと
            // カメラの正面へまっすぐ延びる形になり奥行きが見えず視認しづらいため、
            // カメラ方向へBeamCameraOffset分だけ手前にずらして再生する
            const entt::entity cameraEntity = ::sys::CameraSystem::Get().GetMainCameraEntity();
            const auto* cameraTransform = (cameraEntity != entt::null && registry.valid(cameraEntity))
                ? registry.try_get<ecs::Transform>(cameraEntity)
                : nullptr;

            DirectX::XMFLOAT3 beamRotation = { 0.0f, 0.0f, 0.0f };
            DirectX::XMFLOAT3 beamSpawnPosition = beamPosition;
            if (cameraTransform != nullptr)
            {
                const DirectX::XMFLOAT3& cameraPos = cameraTransform->GetPosition();
                const DirectX::XMFLOAT3 dirToCamera = NormalizeOrZero({
                    cameraPos.x - beamPosition.x,
                    cameraPos.y - beamPosition.y,
                    cameraPos.z - beamPosition.z });

                beamRotation = ComputeBeamRotationFromDirection(dirToCamera);
                beamSpawnPosition =
                {
                    beamPosition.x + dirToCamera.x * masterData.BeamCameraOffset,
                    beamPosition.y + dirToCamera.y * masterData.BeamCameraOffset,
                    beamPosition.z + dirToCamera.z * masterData.BeamCameraOffset,
                };
            }

            ecs::effectutil::PlayOneShotCombined(
                masterData.BeamEffectPath, beamSpawnPosition, masterData.BeamScale,
                &ultimate.BeamEffectEntities, beamRotation);
            return;
        }

        // PlayingBeamフェーズ：ビームの再生終了(またはMaxBeamDurationでのタイムアウト)を待つ
        ultimate.BeamElapsedTime += rawDeltaTime;
        const bool stillPlaying = ecs::effectutil::AnyPlaying(registry, ultimate.BeamEffectEntities);
        const bool timedOut = ultimate.BeamElapsedTime >= masterData.MaxBeamDuration;

        if (stillPlaying && !timedOut) return;

        FinishAndExplode(registry, playerEntity, ultimate, status, masterData);
    }

    /// <summary>プレイヤー正面・低い位置から見上げる構図になるようカメラのTransformを直接更新する</summary>
    void PlayerUltimateSystem::UpdateCamera(
        entt::registry& registry,
        entt::entity playerEntity,
        const ecs::PlayerUltimateComponent& ultimate,
        const data::UltimateData& masterData)
    {
        const entt::entity cameraEntity = ::sys::CameraSystem::Get().GetMainCameraEntity();
        if (cameraEntity == entt::null || !registry.valid(cameraEntity)) return;

        auto* cameraTransform = registry.try_get<ecs::Transform>(cameraEntity);
        const auto* playerTransform = registry.try_get<ecs::Transform>(playerEntity);
        if (cameraTransform == nullptr || playerTransform == nullptr) return;

        const DirectX::XMFLOAT3& start = ultimate.StartPosition;
        const DirectX::XMFLOAT3& forward = ultimate.ForwardDir;

        // 発動時に捕捉したプレイヤーの正面方向へCameraDistance離れた、低い位置(CameraHeight)から
        // 見上げる構図にする。カメラ自体の位置は発動中ずっと固定で、追従はしない
        // (LookAtだけが現在のプレイヤー座標へ追従する)。
        const DirectX::XMFLOAT3 cameraPos =
        {
            start.x + forward.x * masterData.CameraDistance,
            start.y + masterData.CameraHeight,
            start.z + forward.z * masterData.CameraDistance,
        };
        cameraTransform->SetPosition(cameraPos);

        const DirectX::XMFLOAT3& playerPos = playerTransform->GetPosition();
        const DirectX::XMFLOAT3 lookAt =
        {
            playerPos.x,
            playerPos.y + masterData.CameraLookOffset,
            playerPos.z,
        };
        cameraTransform->LookAt(lookAt);
    }

    /// <summary>ビーム終了後：プレイヤー座標・無敵状態を戻し、その場で全体ダメージ+爆発エフェクトを発生させる</summary>
    void PlayerUltimateSystem::FinishAndExplode(
        entt::registry& registry,
        entt::entity playerEntity,
        ecs::PlayerUltimateComponent& ultimate,
        ecs::PlayerStatusComponent& status,
        const data::UltimateData& masterData)
    {
        // ビーム(hougu_pre)の再生が終わったので、非表示化していた他の武器のエフェクトを
        // 元に戻す（これ以降、各武器Systemの発動もIsPlayerUltimateActive()=falseになり再開する）
        SetOtherEffectsVisible(registry, true);

        // 4. プレイヤーの座標を瞬時に発動前の位置へ戻す(テレポート)。
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

        // 5. その場(元の座標)にいる敵全員へ大ダメージ + 爆発エフェクト
        registry.view<ecs::EnemyTag, ecs::EnemyStatusComponent, ecs::Transform>().each(
            [&](ecs::EnemyStatusComponent& enemyStatus, ecs::Transform& enemyTransform)
            {
                enemyStatus.CurrentHp = std::max(0.0f, enemyStatus.CurrentHp - masterData.Damage);
                ecs::combatutil::SpawnDamageNumber(enemyTransform.GetPosition(), masterData.Damage, false);
            });

        // Y=0(発動前の座標そのまま)で再生すると地面に少しめり込むため、ActivationHeightOffset分
        // だけ上げて再生する
        const DirectX::XMFLOAT3 explosionPosition =
        {
            ultimate.StartPosition.x,
            ultimate.StartPosition.y + masterData.ActivationHeightOffset,
            ultimate.StartPosition.z,
        };
        ecs::effectutil::PlayOneShotCombined(
            masterData.ActivationEffectPath, explosionPosition, masterData.ActivationScale);

        ultimate.IsActive = false;
        ultimate.BeamEffectEntities.clear();
    }
}

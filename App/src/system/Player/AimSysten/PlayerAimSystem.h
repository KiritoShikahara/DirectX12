#pragma once

#include<ecs/system/manager/IComponentSystem.h>

namespace sys
{
    /// <summary>
    /// プレイヤーの攻撃方向（PlayerAimComponent::Direction）を更新するシステム。
    ///
    /// 入力デバイスによって決定方法を切り替える。
    ///   InputManager::GetLastInputDevice() == Pad
    ///     → 右スティックの傾き方向をそのまま採用する
    ///   InputManager::GetLastInputDevice() == KeyboardMouse
    ///     → マウスカーソルをプレイヤーの高さの平面に落とし、その点への方向を採用する
    ///
    /// どちらの経路でも方向が確定しなかったフレームでは、
    /// 直前の Direction をそのまま維持する（ゼロベクトルにはしない）。
    ///
    /// Transform の回転には一切触れない。
    /// モデルの向きは RotateToMoveSystem の責務。
    ///
    /// 登録フェーズ: PreUpdate
    /// （Update フェーズの各 FireSystem が Direction を参照するため、それより前に確定させる）
    /// </summary>
    class PlayerAimSystem : public ecs::IUserSystem
    {
    public:
        void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

    private:
        /// <summary>
        /// 右スティックの傾きから攻撃方向を求める。
        /// </summary>
        /// <param name="outDirection">正規化済みの方向（成功時のみ書き込まれる）</param>
        /// <returns>true:方向が確定した false:スティックがデッドゾーン内</returns>
        static bool TryGetPadAimDirection(DirectX::XMFLOAT3& outDirection);

        /// <summary>
        /// マウスカーソルの位置から攻撃方向を求める。
        /// </summary>
        /// <param name="registry">ECSレジストリ</param>
        /// <param name="playerPosition">プレイヤーのワールド座標</param>
        /// <param name="outDirection">正規化済みの方向（成功時のみ書き込まれる）</param>
        /// <returns>true:方向が確定した false:レイが交差しない・カーソルがプレイヤーとほぼ同位置</returns>
        static bool TryGetMouseAimDirection(
            entt::registry& registry,
            const DirectX::XMFLOAT3& playerPosition,
            DirectX::XMFLOAT3& outDirection);

        /// <summary>
        /// 方向が有効とみなす最小の長さの二乗。
        /// これ未満の場合はエイム方向を更新せず、直前の値を維持する。
        /// </summary>
        static constexpr float kMinDirectionLengthSq = 0.0001f;
    };
}
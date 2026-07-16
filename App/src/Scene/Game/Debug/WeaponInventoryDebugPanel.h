#pragma once

#include<string>

namespace debug
{
    /// <summary>
    /// デバッグ用: パーク選択を介さず、プレイヤーの所持武器をImGuiから自由に追加・削除するパネル。
    /// GameSceneFactory::AddWeaponToPlayer/RemoveWeaponFromPlayerを直接呼ぶため、
    /// パーク選択や初期装備と全く同じ経路で武器の生成・破棄が行われる。
    /// IUserSystemは継承せず、生成時にImGuiManagerへ登録・破棄時に解除する。
    /// </summary>
    class WeaponInventoryDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager 登録・解除に使うキー（シーンごとに一意にすること）</param>
        explicit WeaponInventoryDebugPanel(std::string debugKey = "WeaponInventoryDebug");
        ~WeaponInventoryDebugPanel();

        WeaponInventoryDebugPanel(const WeaponInventoryDebugPanel&) = delete;
        WeaponInventoryDebugPanel& operator=(const WeaponInventoryDebugPanel&) = delete;

    private:
        void Draw();

        std::string mDebugKey;
    };
}

#pragma once

#include<string>

namespace debug
{
    ///<summary>
    ///デバッグ用、パーク選択を介さずプレイヤーの所持武器をImGuiから自由に追加・削除するパネル
    ///</summary>
    class WeaponInventoryDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー。シーンごとに一意にすること</param>
        explicit WeaponInventoryDebugPanel(std::string debugKey = "WeaponInventoryDebug");
        ~WeaponInventoryDebugPanel();

        WeaponInventoryDebugPanel(const WeaponInventoryDebugPanel&) = delete;
        WeaponInventoryDebugPanel& operator=(const WeaponInventoryDebugPanel&) = delete;

    private:
        void Draw();

        std::string mDebugKey;
    };
}

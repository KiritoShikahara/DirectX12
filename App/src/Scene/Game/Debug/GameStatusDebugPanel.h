#pragma once

#include<string>

namespace debug
{
    /// <summary>
    /// プレイヤー/敵のランタイムステータス(CurrentHp等)をImGuiで確認するためのデバッグパネル。
    /// EnemyStatusDebugPanel(マスタデータの編集)とは異なり、生成済みエンティティの
    /// 現在値を読み取って表示するだけ。
    /// </summary>
    class GameStatusDebugPanel
    {
    public:
        explicit GameStatusDebugPanel(std::string debugKey = "GameStatusDebug");
        ~GameStatusDebugPanel();

        GameStatusDebugPanel(const GameStatusDebugPanel&) = delete;
        GameStatusDebugPanel& operator=(const GameStatusDebugPanel&) = delete;

    private:
        void Draw();

        // ImGuiManager 登録・解除に使うキー
        std::string mDebugKey;
    };
}

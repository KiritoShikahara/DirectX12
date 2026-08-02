#pragma once

#include<string>

namespace debug
{
    ///<summary>
    ///プレイヤーと敵のランタイムステータスをImGuiで確認するデバッグパネル。生成済みエンティティの現在値を表示するだけ
    ///</summary>
    class GameStatusDebugPanel
    {
    public:
        explicit GameStatusDebugPanel(std::string debugKey = "GameStatusDebug");
        ~GameStatusDebugPanel();

        GameStatusDebugPanel(const GameStatusDebugPanel&) = delete;
        GameStatusDebugPanel& operator=(const GameStatusDebugPanel&) = delete;

    private:
        void Draw();

        ///<summary>
        ///ImGuiManager登録・解除に使うキー
        ///</summary>
        std::string mDebugKey;
    };
}

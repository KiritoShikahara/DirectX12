#pragma once

#include<memory>
#include<string>

namespace data
{
    struct PerkData;
    template<typename T> class DataInspector;
}

namespace debug
{
    /// <summary>
    /// パークのバランス調整データ(PerkData)のデバッグパネル。
    ///
    /// 各パーク種別の抽選重み(Weight)と選択可能回数(MaxLevel)をGUIで編集し、
    /// CSV/DBへ保存できる(DataInspectorのテーブル編集機能をそのまま使う)。
    /// Weightは「その他」枠(3〜5番目の選択肢)の出現率を決める相対値で、
    /// 大きいほど出やすく、0で出現しなくなる。
    ///
    /// 1・2番目の選択肢は新武器獲得・武器レベルアップで枠が固定されているため、
    /// Weightの影響を受けない(PerkSelectSystem::EnterPerkSelect参照)。
    ///
    /// IUserSystemは継承せず、生成時にImGuiManagerへ登録・破棄時に解除する。
    /// </summary>
    class PerkDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager 登録・解除に使うキー（シーンごとに一意にすること）</param>
        explicit PerkDebugPanel(std::string debugKey = "PerkDebug");
        ~PerkDebugPanel();

        PerkDebugPanel(const PerkDebugPanel&) = delete;
        PerkDebugPanel& operator=(const PerkDebugPanel&) = delete;

    private:
        void Draw();

        // テーブルエディタ（Id含む全フィールド編集・CSV/DB操作）
        std::unique_ptr<data::DataInspector<data::PerkData>> mInspector;

        // ImGuiManager 登録・解除に使うキー
        std::string mDebugKey;
    };
}

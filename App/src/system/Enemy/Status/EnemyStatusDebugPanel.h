#pragma once

#include<memory>
#include<string>

namespace data
{
    struct EnemyData;
    template<typename T> class DataInspector;
}

namespace debug
{
    /// <summary>
    /// 謨ｵ繝槭せ繧ｿ(EnemyData)縺ｮ繝・ヰ繝・げ繝代ロ繝ｫ縲・
    /// 譌｢蟄・DataInspector 縺ｫ繧医ｋ繝・・繝悶Ν邱ｨ髮・CSV/DB繝ｻId蜷ｫ繧蜈ｨ繝輔ぅ繝ｼ繝ｫ繝・縺ｫ蜉縺医・
    /// 邱ｨ髮・・螳ｹ繧堤函蟄倅ｸｭ縺ｮ謨ｵ(EnemyStatusComponent)縺ｸ蜀埼←逕ｨ縺吶ｋ讖溯・繧呈戟縺､縲・
    /// IUserSystem 縺ｯ邯呎価縺帙★縲∫函謌先凾縺ｫ ImGuiManager 縺ｸ逋ｻ骭ｲ繝ｻ遐ｴ譽・凾縺ｫ隗｣髯､縺吶ｋ縲・
    /// </summary>
    class EnemyStatusDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager 逋ｻ骭ｲ繝ｻ隗｣髯､縺ｫ菴ｿ縺・く繝ｼ・医す繝ｼ繝ｳ縺斐→縺ｫ荳諢上↓縺吶ｋ縺薙→・・/param>
        explicit EnemyStatusDebugPanel(std::string debugKey = "EnemyStatusDebug");
        ~EnemyStatusDebugPanel();

        EnemyStatusDebugPanel(const EnemyStatusDebugPanel&) = delete;
        EnemyStatusDebugPanel& operator=(const EnemyStatusDebugPanel&) = delete;

    private:
        void Draw();
        void ApplyAllToEnemies();
        void ApplyRowToEnemies(const data::EnemyData& row);

        // 譌｢蟄倥・繝・・繝悶Ν繧ｨ繝・ぅ繧ｿ・・d蜷ｫ繧蜈ｨ繝輔ぅ繝ｼ繝ｫ繝臥ｷｨ髮・・CSV/DB謫堺ｽ懊ｒ蜀・桁・・
        std::unique_ptr<data::DataInspector<data::EnemyData>> mInspector;

        // ImGuiManager 逋ｻ骭ｲ繝ｻ隗｣髯､縺ｫ菴ｿ縺・く繝ｼ
        std::string mDebugKey;
    };
}
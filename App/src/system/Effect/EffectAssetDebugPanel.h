#pragma once

#include<memory>
#include<string>

namespace data
{
    struct EffectAssetData;
    template<typename T> class DataInspector;
}

namespace debug
{
    ///<summary>
    ///エフェクト素材ID→パス解決テーブルのマスタデータEffectAssetDataのデバッグパネル。DataInspectorによるテーブル編集をそのまま提供する
    ///</summary>
    class EffectAssetDebugPanel
    {
    public:
        /// <param name="debugKey">ImGuiManager登録・解除に使うキー。シーンごとに一意にすること</param>
        explicit EffectAssetDebugPanel(std::string debugKey = "EffectAssetDebug");
        ~EffectAssetDebugPanel();

        EffectAssetDebugPanel(const EffectAssetDebugPanel&) = delete;
        EffectAssetDebugPanel& operator=(const EffectAssetDebugPanel&) = delete;

    private:
        void Draw();

        std::unique_ptr<data::DataInspector<data::EffectAssetData>> mInspector;
        std::string mDebugKey;
    };
}

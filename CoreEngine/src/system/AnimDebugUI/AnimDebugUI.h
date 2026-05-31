#pragma once
#include <entt/entt.hpp>
#include <Utility/Export/Export.h>

namespace sys
{
    /// <summary>
    /// アニメーション状態を ImGui でリアルタイム確認するデバッグ UI
    ///
    /// 表示内容:
    ///   - クリップ一覧 (名前・長さ・baked/sparse)
    ///   - 再生状態 (クリップ番号・現在時刻・ブレンド状態)
    ///   - ボーン一覧と親子関係
    ///   - アニメーショントラックのボーン名解決結果 (緑=OK 赤=NOT FOUND)
    ///   - 先頭5本のボーン行列の値
    ///
    /// 使い方:
    ///   AnimationDebugUI::Register(registry);  // 初期化時に一度だけ
    /// </summary>
    class ENGINE_API AnimationDebugUI
    {
    public:
        static void Register(entt::registry& registry);
    };

} // namespace sys
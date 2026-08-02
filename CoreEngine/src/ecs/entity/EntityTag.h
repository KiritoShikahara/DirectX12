#pragma once

namespace ecs
{
    // 永久オブジェクト
    struct PersistentTag {};

    // 破棄する予定のオブジェクトのタグ
    struct PendingDestroyTag {};

    // 選択されているオブジェクトに付けるタグ
    struct SelectedTag {};

    // 描画するオブジェクトに付けるタグ
    struct RenderableTag {};

    // エディタで配置されたエンティティ。Save/Load/スナップショットの対象になる。
    struct PlaceableTag {};
}
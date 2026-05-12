#pragma once

namespace ecs
{
    // シーンを跨いで生存する
    struct PersistentTag {};

    // 現在のフレームの最後に削除される予定
    struct PendingDestroyTag {};

    // 編集中のエディタ上でのみ選択されている
    struct SelectedTag {};

    // カメラに映る対象である
    struct RenderableTag {};
}
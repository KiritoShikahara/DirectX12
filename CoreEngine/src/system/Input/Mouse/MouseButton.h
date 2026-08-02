#pragma once

namespace sys
{
    /// <summary>
    /// マウスボタンコード定義
    /// </summary>
    enum class eMouseButton : int
    {
        Unknown = -1,
        Left = 0,
        Right,
        Middle,
        XButton1,
        XButton2,
        Count,
    };
}
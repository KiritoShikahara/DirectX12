#pragma once

namespace ecs
{
    /// <summary>
    /// 一定時間後に自動的にエンティティを破棄するための汎用コンポーネント。
    /// DebugWireSphereComponentのように、Effekseerのエフェクト再生(autoDelete)に
    /// 紐付かない一時エンティティの後始末に使う（TemporaryLifetimeSystemが処理する）。
    /// </summary>
    struct TemporaryLifetimeComponent
    {
        /// <summary>残り時間(秒)。0以下になったエンティティはTemporaryLifetimeSystemが破棄する</summary>
        float RemainingTime = 1.0f;
    };
}

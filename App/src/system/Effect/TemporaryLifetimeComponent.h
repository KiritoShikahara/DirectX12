#pragma once

namespace ecs
{
    ///<summary>
    ///一定時間後に自動的にエンティティを破棄するための汎用コンポーネント。EffekseerのautoDeleteに紐付かない一時エンティティの後始末に使う
    ///</summary>
    struct TemporaryLifetimeComponent
    {
        ///<summary>
        ///残り時間、秒。0以下になったエンティティはTemporaryLifetimeSystemが破棄する
        ///</summary>
        float RemainingTime = 1.0f;
    };
}

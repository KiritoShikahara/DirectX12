#pragma once

#include"../IScene.h"

namespace sys
{
    /// <summary>
    /// SceneFactory で名前が見つからなかったときに使われるフォールバックシーン。
    /// Initialize / Finalize は何もしない。
    /// </summary>
    class DefaultScene : public IScene
    {
    public:
        void Initialize() override {}
        void Finalize()   override {}
    };
}
#pragma once

namespace sys
{
    /// <summary>
    /// シーン基底インターフェース
    /// 全シーンはこのクラスを継承して Initialize / Finalize を実装する
    /// </summary>
    class IScene
    {
    public:
        virtual ~IScene() = default;

        /// <summary>
        /// シーン開始時に一度だけ呼ばれる
        /// リソースのロード、ECS エンティティの生成などを行う
        /// </summary>
        virtual void Initialize() = 0;

        /// <summary>
        /// シーン終了時に一度だけ呼ばれる
        /// リソースの解放、ECS エンティティの削除などを行う
        /// </summary>
        virtual void Finalize() = 0;
    };
}
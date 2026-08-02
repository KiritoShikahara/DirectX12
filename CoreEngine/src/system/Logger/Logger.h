#pragma once

#include <Utility/Export/Export.h>
#include <Utility/Singleton/Singleton.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>
#include <format>

namespace sys
{
    /// <summary>
    /// ログの出力レベル。値が大きいほど重篤。
    /// </summary>
    enum class ENGINE_API eLogLevel : uint8_t
    {
        /// <summary>通常ログ</summary>
        Log,
        /// <summary>警告ログ</summary>
        Warning,
        /// <summary>エラーログ</summary>
        Error,
        /// <summary>致命的エラー。その場でプログラムを停止する。</summary>
        Fatal
    };

    /// <summary>
    /// フォーマット文字列と呼び出し元情報をまとめた構造体。
    /// </summary>
    struct ENGINE_API LogMessage
    {
        std::string_view     fmt;
        std::source_location location;

        /// <summary>
        /// 暗黙変換コンストラクタ。
        /// 文字列リテラルを渡すだけで location が自動取得される
        /// </summary>
        LogMessage(
            std::string_view     f,
            std::source_location loc = std::source_location::current())
            : fmt(f), location(loc)
        {
        }
    };

    /// <summary>
    /// ログの書き出し先を抽象化するインターフェース。
    /// </summary>
    class ENGINE_API ILogSink
    {
    public:
        virtual ~ILogSink() = default;

        /// <summary>
        /// ログを書き出す。Logger の内部ミューテックスが掛かった状態で呼ばれる。
        /// </summary>
        virtual void Write(
            eLogLevel                   level,
            std::string_view             message,
            const std::source_location& location) = 0;
    };

    /// <summary>
    /// ログを各 ILogSink へ配送するクラス。
    /// </summary>
    class ENGINE_API Logger : public utility::Singleton<Logger>
    {
        SINGLETON_CLASS(Logger);
    public:
        SINGLETON_ACCESSOR(Logger);

        /// <summary>
        /// 初期化
        /// </summary>
        bool Initialize();

        /// <summary>
        /// 終了処理
        /// </summary>
        void Finalize();

        /// <summary>
        /// シンクを追加する。スレッドセーフ。
        /// </summary>
        void AddSink(std::unique_ptr<ILogSink> sink);

        /// <summary>
        /// ログを出力する。ECSE_LOG マクロ経由での使用を推奨。
        /// </summary>
        template<typename... Args>
        void Output(eLogLevel level, LogMessage msg, Args&&... args)
        {
            try
            {
                std::string message = std::vformat(
                    msg.fmt,
                    std::make_format_args(args...));

                Dispatch(level, message, msg.location);
            }
            catch (const std::format_error& e)
            {
                std::string errMsg = std::string("FormatError: ") + e.what();
                Dispatch(eLogLevel::Error, errMsg, msg.location);
            }
        }

    private:
        /// <summary>
        /// 全シンクへ配送する。
        /// </summary>
        void Dispatch(
            eLogLevel                   level,
            const std::string& message,
            const std::source_location& location);

        static std::mutex                       sMutex;
        std::vector<std::unique_ptr<ILogSink>>      mSinks;
    };

    /// <summary>
    /// Visual Studio の出力ウィンドウへ OutputDebugStringA で書き出すシンク。
    /// </summary>
    class ENGINE_API DebugOutputSink : public ILogSink
    {
    public:
        void Write(
            eLogLevel                   level,
            std::string_view             message,
            const std::source_location& location) override;
    };

    /// <summary>
    /// AllocConsole で生成したコンソールウィンドウへ書き出すシンク。
    /// </summary>
    class ENGINE_API ConsoleLogSink : public ILogSink
    {
    public:
        explicit ConsoleLogSink(bool allocConsole);
        ~ConsoleLogSink() override;

        void Write(
            eLogLevel                   level,
            std::string_view             message,
            const std::source_location& location) override;

    private:
        void SetTextColor(eLogLevel level);
        void ResetTextColor();

        bool mOwnsConsole = false;
    };

    /// <summary>
    /// テキストファイルへタイムスタンプ付きで追記するシンク
    /// </summary>
    class ENGINE_API FileLogSink : public ILogSink
    {
    public:
        explicit FileLogSink(std::string_view filePath);
        ~FileLogSink() override;

        void Write(
            eLogLevel                   level,
            std::string_view             message,
            const std::source_location& location) override;

    private:
        FILE* mFile = nullptr;
    };

    /// <summary>
    /// コンパイル時にログレベルの有効・無効を判定する。
    /// </summary>
    constexpr bool IsLogLevelEnabled([[maybe_unused]] eLogLevel level) noexcept
    {
#ifdef _DEBUG
        return true;
#else
        return level >= eLogLevel::Error;
#endif
    }
}

#define DEBUG_LOG(level, format, ...) \
    do { \
        if constexpr (::sys::IsLogLevelEnabled(level)) { \
            ::sys::Logger::Get().Output( \
                (level), \
                ::sys::LogMessage{ (format) }, \
                ##__VA_ARGS__); \
        } \
    } while (0)

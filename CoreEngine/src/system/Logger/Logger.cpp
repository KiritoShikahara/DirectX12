#include"pch.h"
#include "Logger.h"

namespace sys
{
	namespace
	{
		/// <summary>
		/// ログレベルの表示ラベルを返す。幅を揃えて整列させる。
		/// </summary>
		constexpr std::string_view LevelLabel(eLogLevel level) noexcept
		{
			switch (level)
			{
			case eLogLevel::Warning: return "[Warning]";
			case eLogLevel::Error:   return "[Error]  ";
			case eLogLevel::Fatal:   return "[FATAL]  ";
			default:                 return "[Log]    ";
			}
		}
	} // anonymous namespace

	std::mutex Logger::sMutex;

	bool Logger::Initialize()
	{
		// VSの出力ウィンドウ（デバッグ用）
		AddSink(std::make_unique<DebugOutputSink>());

		// コンソールシンクはデバッグビルドのみ
#if defined(_DEBUG) || ECSE_DEV_TOOL_ENABLED
		AddSink(std::make_unique<ConsoleLogSink>(true));
#endif

		// ファイルシンクは常に登録する（実行ログの保存）
		AddSink(std::make_unique<FileLogSink>("ecse_log.txt"));

		DEBUG_LOG(eLogLevel::Log, "Logger initialized.");

		return true;
	}

	void Logger::Finalize()
	{
		// TODO:ログ出力
		DEBUG_LOG(eLogLevel::Log, "Logger finalized.");

		std::lock_guard lock(sMutex);
		mSinks.clear();
	}

	/// <summary>
	/// シンクを追加する。スレッドセーフ。
	/// </summary>
	void Logger::AddSink(std::unique_ptr<ILogSink> sink)
	{
		std::lock_guard lock(sMutex);
		mSinks.emplace_back(std::move(sink));
	}
	
	/// <summary>
	/// 全シンクへ配送する。Fatal の場合は MessageBox + DebugBreak も行う。
	/// </summary>
	void Logger::Dispatch(eLogLevel level, const std::string& message, const std::source_location& location)
	{
		{
			std::lock_guard lock(sMutex);
			for (auto& sink : mSinks)
			{
				sink->Write(level, message, location);
			}
		}

		// Fatal の後処理はシンクの外で行う（ミューテックスを手放してから）
		if (level == eLogLevel::Fatal)
		{
			std::string detail = std::format(
				"{}\n\nLocation: {}({})",
				message,
				location.file_name(),
				location.line());

			MessageBoxA(nullptr, detail.c_str(), "Fatal Error", MB_ICONERROR);
			DebugBreak();
		}

	}

	/// <summary>
	/// Visual Studio の出力ウィンドウへ OutputDebugStringA で書き出すシンク。
	/// </summary>
	void DebugOutputSink::Write(eLogLevel level, std::string_view message, const std::source_location& location)
	{
		std::string out;

		// ファイル名と行番号は Warning 以上のみ付与する
		if (level != eLogLevel::Log)
		{
			out = std::format("{}{} [{}({})]\n",
				LevelLabel(level), message,
				location.file_name(), location.line());
		}
		else
		{
			out = std::format("{}{}\n", LevelLabel(level), message);
		}

		OutputDebugStringA(out.c_str());

	}

	ConsoleLogSink::ConsoleLogSink(bool allocConsole)
	{
		if (allocConsole && AllocConsole() != 0)
		{
			FILE* fp = nullptr;
			freopen_s(&fp, "CONOUT$", "w", stdout);
			freopen_s(&fp, "CONIN$", "r", stdin);
			std::setlocale(LC_ALL, "japanese");
			mOwnsConsole = true;
		}
	}

	ConsoleLogSink::~ConsoleLogSink()
	{
		if (mOwnsConsole)
		{
			FreeConsole();
		}
	}

	/// <summary>
	/// AllocConsole で生成したコンソールウィンドウへ書き出すシンク。
	/// デバッグビルドでのみ有効。
	/// </summary>
	void ConsoleLogSink::Write(eLogLevel level, std::string_view message, const std::source_location& location)
	{
		SetTextColor(level);

		std::cout << LevelLabel(level) << message << '\n';

		// 発生箇所は Warning 以上のみ表示する
		if (level != eLogLevel::Log)
		{
			std::cout << "  -> " << location.file_name()
				<< '(' << location.line() << ")\n";
		}

		ResetTextColor();

	}

	void ConsoleLogSink::SetTextColor(eLogLevel level)
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		WORD   color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // 白（デフォルト）

		switch (level)
		{
		case eLogLevel::Warning:
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;  // 黄
			break;
		case eLogLevel::Error:
			color = FOREGROUND_RED | FOREGROUND_INTENSITY;                     // 赤
			break;
		case eLogLevel::Fatal:
			color = BACKGROUND_RED |
				FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
				FOREGROUND_INTENSITY;                                       // 赤背景・白文字
			break;
		default:
			break;
		}

		SetConsoleTextAttribute(h, color);

	}

	void ConsoleLogSink::ResetTextColor()
	{
		SetConsoleTextAttribute(
			GetStdHandle(STD_OUTPUT_HANDLE),
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}

	FileLogSink::FileLogSink(std::string_view filePath)
	{
		fopen_s(&mFile, filePath.data(), "a");
	}

	FileLogSink::~FileLogSink()
	{
		if (mFile != nullptr)
		{
			fclose(mFile);
			mFile = nullptr;
		}
	}

	/// <summary>
	/// テキストファイルへタイムスタンプ付きで追記するシンク
	/// </summary>
	void FileLogSink::Write(eLogLevel level, std::string_view message, const std::source_location& location)
	{
		if (mFile == nullptr) return;

		// タイムスタンプを生成する
		const auto now = std::chrono::system_clock::now();
		const auto timeStr = std::format("{:%Y-%m-%d %H:%M:%S}", now);

		std::string line;
		if (level != eLogLevel::Log)
		{
			line = std::format("[{}] {}{} [{}({})]\n",
				timeStr,
				LevelLabel(level),
				message,
				location.file_name(),
				location.line());
		}
		else
		{
			line = std::format("[{}] {}{}\n",
				timeStr,
				LevelLabel(level),
				message);
		}

		fputs(line.c_str(), mFile);
		fflush(mFile);  // プロセスが異常終了しても書き出す

	}

} // namespace sys
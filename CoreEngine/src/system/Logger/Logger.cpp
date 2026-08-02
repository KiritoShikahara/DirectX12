#include"pch.h"
#include "Logger.h"

namespace sys
{
	namespace
	{
		// ログレベルの文字表示をそろえて返す
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
		// VSの出力ウィンドへ出力
		AddSink(std::make_unique<DebugOutputSink>());

		// コンソール用
#if DEV_TOOL_ENABLED
		AddSink(std::make_unique<ConsoleLogSink>(true));
#endif
		// ファイル出力
		AddSink(std::make_unique<FileLogSink>("ecse_log.txt"));

		DEBUG_LOG(eLogLevel::Log, "Logger initialized.");

		return true;
	}

	void Logger::Finalize()
	{
		DEBUG_LOG(eLogLevel::Log, "Logger finalized.");

		std::lock_guard lock(sMutex);
		mSinks.clear();
	}

	// シンクの追加
	void Logger::AddSink(std::unique_ptr<ILogSink> sink)
	{
		std::lock_guard lock(sMutex);
		mSinks.emplace_back(std::move(sink));
	}

	// 全シンクへの配信
	void Logger::Dispatch(eLogLevel level, const std::string& message, const std::source_location& location)
	{
		{
			std::lock_guard lock(sMutex);
			for (auto& sink : mSinks)
			{
				sink->Write(level, message, location);
			}
		}

		// Fatal の後処理の大部分はシンクの外で行う
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


	// VSの出力ウィンドへ文字列を出力するシンク
	void DebugOutputSink::Write(eLogLevel level, std::string_view message, const std::source_location& location)
	{
		std::string out;

		// ファイル名と行番号はwarnig以上のみ付与する
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


	// AllocConsole で生成したコンソールウィンドウへ文字出力するシンク。デバッグビルドでのみ
	void ConsoleLogSink::Write(eLogLevel level, std::string_view message, const std::source_location& location)
	{
		SetTextColor(level);

		std::cout << LevelLabel(level) << message << '\n';

		// 発信場所は Warning 以上のみ表示する
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
		WORD   color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // デフォルト

		switch (level)
		{
		case eLogLevel::Warning:
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;  // 黄色
			break;
		case eLogLevel::Error:
			color = FOREGROUND_RED | FOREGROUND_INTENSITY;                     // 赤色
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
		// 起動のたびにリセットし、最後に起動した1回分のログのみを残す（"a"=追記だと肥大化し続けるため）
		fopen_s(&mFile, filePath.data(), "w");
	}

	FileLogSink::~FileLogSink()
	{
		if (mFile != nullptr)
		{
			fclose(mFile);
			mFile = nullptr;
		}
	}


	// テキストファイルへタイムスタンプ付きで追記するシンク
	void FileLogSink::Write(eLogLevel level, std::string_view message, const std::source_location& location)
	{
		if (mFile == nullptr) return;

		// タイムスタンプ
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
		fflush(mFile); // プロセス異常終了時にもログ出力されるようにする。

	}

} // namespace sys
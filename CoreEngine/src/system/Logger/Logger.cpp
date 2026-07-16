#include"pch.h"
#include "Logger.h"

namespace sys
{
	namespace
	{
		/// <summary>
		/// ���O���x���̕\�����x����Ԃ��B���𑵂��Đ��񂳂���B
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
		// VS�̏o�̓E�B���h�E�i�f�o�b�O�p�j
		AddSink(std::make_unique<DebugOutputSink>());

		// �R���\�[���V���N�̓f�o�b�O�r���h�̂�
#if defined(_DEBUG) || ECSE_DEV_TOOL_ENABLED
		AddSink(std::make_unique<ConsoleLogSink>(true));
#endif

		// �t�@�C���V���N�͏�ɓo�^����i���s���O�̕ۑ��j
		AddSink(std::make_unique<FileLogSink>("ecse_log.txt"));

		DEBUG_LOG(eLogLevel::Log, "Logger initialized.");

		return true;
	}

	void Logger::Finalize()
	{
		// TODO:���O�o��
		DEBUG_LOG(eLogLevel::Log, "Logger finalized.");

		std::lock_guard lock(sMutex);
		mSinks.clear();
	}

	/// <summary>
	/// �V���N��ǉ�����B�X���b�h�Z�[�t�B
	/// </summary>
	void Logger::AddSink(std::unique_ptr<ILogSink> sink)
	{
		std::lock_guard lock(sMutex);
		mSinks.emplace_back(std::move(sink));
	}
	
	/// <summary>
	/// �S�V���N�֔z������BFatal �̏ꍇ�� MessageBox + DebugBreak ���s���B
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

		// Fatal �̌㏈���̓V���N�̊O�ōs���i�~���[�e�b�N�X��������Ă���j
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
	/// Visual Studio �̏o�̓E�B���h�E�� OutputDebugStringA �ŏ����o���V���N�B
	/// </summary>
	void DebugOutputSink::Write(eLogLevel level, std::string_view message, const std::source_location& location)
	{
		std::string out;

		// �t�@�C�����ƍs�ԍ��� Warning �ȏ�̂ݕt�^����
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
	/// AllocConsole �Ő��������R���\�[���E�B���h�E�֏����o���V���N�B
	/// �f�o�b�O�r���h�ł̂ݗL���B
	/// </summary>
	void ConsoleLogSink::Write(eLogLevel level, std::string_view message, const std::source_location& location)
	{
		SetTextColor(level);

		std::cout << LevelLabel(level) << message << '\n';

		// �����ӏ��� Warning �ȏ�̂ݕ\������
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
		WORD   color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // ���i�f�t�H���g�j

		switch (level)
		{
		case eLogLevel::Warning:
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;  // ��
			break;
		case eLogLevel::Error:
			color = FOREGROUND_RED | FOREGROUND_INTENSITY;                     // ��
			break;
		case eLogLevel::Fatal:
			color = BACKGROUND_RED |
				FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
				FOREGROUND_INTENSITY;                                       // �Ԕw�i�E������
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

	/// <summary>
	/// �e�L�X�g�t�@�C���փ^�C���X�^���v�t���ŒǋL����V���N
	/// </summary>
	void FileLogSink::Write(eLogLevel level, std::string_view message, const std::source_location& location)
	{
		if (mFile == nullptr) return;

		// �^�C���X�^���v�𐶐�����
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
		fflush(mFile);  // �v���Z�X���ُ�I�����Ă������o��

	}

} // namespace sys
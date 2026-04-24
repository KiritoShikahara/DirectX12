#pragma once

#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>

#include<cstdint>
#include<cstdio>
#include<string>
#include<string_view>
#include<mutex>
#include <source_location>


namespace sys
{

	/// <summary>
	/// Debug出力レベル定義
	/// </summary>
	enum class ENGINE_API eLogLevel : uint8_t
	{
		/// <summary>
		/// 通常ログ 
		/// 出力先:出力ログ
		/// </summary>
		Log,

		/// <summary>
		/// 警告ログ 
		/// 出力先:出力ログ, コンソール画面
		/// </summary>
		Warning,

		/// <summary>
		/// エラーログ 
		/// 出力先:出力ログ, コンソール画面
		/// </summary>
		Error,

		/// <summary>
		/// 致命的エラー(実行をその場で止める) 
		/// 出力先:出力ログ, コンソール画面
		/// </summary>
		Fatal

	};

	/// <summary>
	/// コンソールの文字の色変えるための定義
	/// </summary>
	enum class ENGINE_API eConsoleTextColor : uint8_t
	{
		Red = 0x01,
		Blue = 0x02,
		Green = 0x04,
		Yellow = Red | Green,
		Purple = Red | Blue,
		Cyan = Blue | Green,
		White = Red | Blue | Green,
	};

	/// <summary>
	/// ログ管理
	/// </summary>
	class ENGINE_API Logger : public utility::Singleton<Logger>
	{
		SINGLETON_CLASS(Logger);
	public:
		SINGLETON_ACCESSOR(Logger);
	private:
		/// <summary>
		/// コンソールのリソース取得
		/// </summary>
		/// <returns></returns>
		bool Initialize();
		
		/// <summary>
		/// コンソールのリソース解放
		/// </summary>
		void Finalize();

		
	};
}


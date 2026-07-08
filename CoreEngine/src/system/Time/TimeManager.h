#pragma once

#include"Time.h"
#include<Utility/Singleton/Singleton.hpp>

namespace sys
{
	class TimeManager : public utility::Singleton<TimeManager>
	{
		SINGLETON_CLASS(TimeManager);
	public:
		SINGLETON_ACCESSOR(TimeManager);

		/// <summary>
		/// 初期化
		/// </summary>
		/// <returns></returns>
		bool Initialize()
		{
			mTime.Initialize();

			return true;
		}

		/// <summary>
		/// 書き換え用の取得メソッド
		/// </summary>
		Time& GetTime() { return mTime; }

		/// <summary>
		/// 参照のみの取得メソッド
		/// </summary>
		const Time& GetTime() const { return mTime; }

	private:
		::sys::Time mTime;
	};
}

/// <summary>
/// 時間クラス参照取得
/// </summary>
/// <returns></returns>
inline ::sys::Time& GetTime()
{
	return ::sys::TimeManager::Get().GetTime();
}



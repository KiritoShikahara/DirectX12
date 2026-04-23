#pragma once
#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>

namespace sys
{
	struct WindowContext;

	/// <summary>
	/// ウィンドウ関係の管理
	/// </summary>
	class ENGINE_API Window : public utility::Singleton<Window>
	{
		SINGLETON_CLASS(Window);
	public:
		SINGLETON_ACCESSOR(Window);

		bool Initialize(const WindowContext& a_context);

	private:

	};
}


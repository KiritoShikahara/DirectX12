#pragma once
#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>

namespace sys
{
	class Window;
	struct EngineContext;

	class ENGINE_API Engine : public utility::Singleton<Engine>
	{
		Engine();

		SINGLETON_CLASS_CUSTOM_CTOR(Engine);
	public:
		SINGLETON_ACCESSOR(Engine);

		/// <summary>
		/// App初期化
		/// </summary>
		/// <param name="context">初期化情報</param>
		/// <returns>true:成功 false:失敗</returns>
		bool Initialize(EngineContext context);

		/// <summary>
		/// Appの実行
		/// </summary>
		bool Run();

		/// <summary>
		/// App終了処理
		/// </summary>
		/// <returns>true:成功 false:失敗</returns>
		bool Finalize();

	private:
		/// <summary>
		/// 実行中フラグ　true:実行中 false:終了
		/// </summary>
		bool mIsRunning;

		/// <summary>
		/// 初期化完了フラグ　true:初期化完了 false:未初期化
		/// </summary>
		bool mIsInitialized;

	private:
		/// <summary>
		/// ウィンドウ管理クラス
		/// </summary>
		Window* mWindow;

	};
}

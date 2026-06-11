#pragma once
#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>
#include<system/Time/Time.h>
#include<graphics/Dx12/Dx12Renderer.h>
#include<memory>

namespace graphics
{
	class DX12Device;
	class DX12Renderer;
}

namespace ecs
{
	class EntityManager;
	class ComponentSystemManager;
}

namespace sys
{
	class Window;
	struct EngineContext;
	class ImGuiManager;
	class InputManager;
	class SceneManager;

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
		/// デバックUIの初期化
		/// </summary>
		void InitializeDebugUI();

		/// <summary>
		/// 状態更新
		/// </summary>
		void Update();

		/// <summary>
		/// 描画
		/// </summary>
		void Render();

		/// <summary>
		/// フレーム末の処理
		/// </summary>
		void Conclude();

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
		/// 時間管理
		/// </summary>
		sys::Time mTime;

		/// <summary>
		/// ウィンドウ管理クラス
		/// </summary>
		Window* mWindow;

		/// <summary>
		/// DX12デバイス管理
		/// </summary>
		graphics::DX12Device* mDevice;

		/// <summary>
		/// DX12レンダー用コンテキスト管理
		/// </summary>
		graphics::DX12Renderer* mDX12Renderer;

		/// <summary>
		/// ImGui管理クラス
		/// </summary>
		ImGuiManager* mImGuiManager;

		/// <summary>
		/// エンティティ管理クラス
		/// </summary>
		ecs::EntityManager* mEntityManager;

		/// <summary>
		/// コンポーネントのシステム管理
		/// </summary>
		ecs::ComponentSystemManager* mComponentSystemManager;

		/// <summary>
		/// 入力管理クラス
		/// </summary>
		InputManager* mInputManager;

		/// <summary>
		/// シーン管理
		/// </summary>
		SceneManager* mSceneManager;
	};
}

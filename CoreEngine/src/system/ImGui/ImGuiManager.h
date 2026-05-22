#pragma once
#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>
#include<vector>


namespace graphics
{
	class DX12Device;
	class DX12Context;
	class GDescriptorHeapManager;
}

namespace sys
{
	class Window;
}

namespace sys
{
	class ImGuiManager : public utility::Singleton<ImGuiManager>
	{
		SINGLETON_CLASS(ImGuiManager);
	public:
		SINGLETON_ACCESSOR(ImGuiManager);

		/// <summary>
		/// ImGui初期化
		/// </summary>
		/// <returns>true:成功 false:失敗</returns>
		bool Initialize(
			sys::Window&window,
			graphics::DX12Device& device,
			graphics::DX12Context& renderer,
			graphics::GDescriptorHeapManager& descriptorHeapManager
		);

		/// <summary>
		/// 終了処理
		/// </summary>
		void Finalize();

		/// <summary>フレーム開始。BeginRendering の直後に呼ぶ。</summary>
		void NewFrame();

		/// <summary>
		/// 登録された UI 関数を順に呼び出す。
		/// NewFrame() と EndFrame() の間に呼ぶこと。
		/// </summary>
		void Update();

		/// <summary>
		/// 描画データの確定と ImGui コマンドの発行。
		/// Flip() の直前に呼ぶ。
		/// </summary>
		void EndFrame();

		/// <summary>
		/// デバッグ UI 描画関数を登録する。
		/// 登録した関数は Update() 内で毎フレーム呼ばれる。
		/// </summary>
		void AddDebugUI(std::function<void()> guiFunc);
	private:
		/// <summary>登録済みのデバッグ UI 描画関数リスト</summary>
		std::vector<std::function<void()>> mDebugUIFunctions;

		/// <summary>
		/// フォント用ディスクリプタスロット
		/// </summary>
		graphics::GDescriptorHeap          mFontHeap;

		/// <summary>
		/// EndFrame() で毎フレーム使うコマンドリストの供給元。
		/// ライフタイムは Engine 側が保証する前提でポインタ保持。
		/// </summary>
		graphics::DX12Context* mRenderer = nullptr;

		/// <summary>
		/// EndFrame() で SetDescriptorHeaps に渡すネイティブヒープの供給元。
		/// </summary>
		graphics::GDescriptorHeapManager* mHeapManager = nullptr;

		/// <summary>ImGui コンテキスト</summary>
		ImGuiContext* mContext = nullptr;

		bool                                mIsInitialized = false;
	};
}



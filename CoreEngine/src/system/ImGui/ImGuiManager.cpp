#include "pch.h"
#include "ImGuiManager.h"

#include<system/Window/Window.h>
#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Dx12/Dx12Context.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>

#include<ImGui/imgui.h>
#include<ImGui/imgui_impl_dx12.h>
#include<ImGui/imgui_impl_win32.h>

namespace sys
{

	/// <summary>
	/// ImGui初期化
	/// </summary>
	/// <returns>true:成功 false:失敗</returns>
	bool ImGuiManager::Initialize(sys::Window& window, graphics::DX12Device& device, graphics::DX12Context& context, graphics::GDescriptorHeapManager& descriptorHeapManager)
	{
		IMGUI_CHECKVERSION();
		mContext = ImGui::CreateContext();

		if (mContext == nullptr)
		{
			DEBUG_LOG(sys::eLogLevel::Error, "ImGuiManager: Failed to create ImGui context.");
			return false;
		}

		ImGui::SetCurrentContext(mContext);

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		// ---- Win32 バックエンドの初期化 ----
		if (!ImGui_ImplWin32_Init(window.GetHWND()))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "ImGuiManager: Failed to init Win32 backend.");
			return false;
		}

		if (!mFontHeap.Create(descriptorHeapManager, 1))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"ImGuiManager: Failed to create font descriptor heap.");
			return false;
		}

		// ---- DX12 バックエンドの初期化 ----
		ImGui_ImplDX12_InitInfo initInfo = {};
		initInfo.Device = device.GetDevice();
		initInfo.CommandQueue = context.GetCommandQueue();
		initInfo.NumFramesInFlight = graphics::FRAME_COUNT;
		initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN; // ImGui は深度バッファを使わない

		initInfo.SrvDescriptorHeap = descriptorHeapManager.GetNativeHeap();
		initInfo.LegacySingleSrvCpuDescriptor = mFontHeap.GetCpuHandle();
		initInfo.LegacySingleSrvGpuDescriptor = mFontHeap.GetGpuHandle();

		if (!ImGui_ImplDX12_Init(&initInfo))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "ImGuiManager: Failed to init DX12 backend.");
			return false;
		}

		// EndFrame() で毎フレーム使うものだけ保持する
		mRendererContext = &context;
		mHeapManager = &descriptorHeapManager;

		mIsInitialized = true;
		DEBUG_LOG(sys::eLogLevel::Log, "ImGuiManager: Initialized successfully.");
		return true;
	}

	/// <summary>
	/// 終了処理
	/// </summary>
	void ImGuiManager::Finalize()
	{
		if (!mIsInitialized) return;

		// ImGui バックエンドの終了
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();

		if (mContext != nullptr)
		{
			ImGui::DestroyContext(mContext);
			mContext = nullptr;
		}
		mRendererContext = nullptr;
		mHeapManager = nullptr;
		mIsInitialized = false;

		DEBUG_LOG(sys::eLogLevel::Log, "ImGuiManager finalized.");
	}

	/// <summary>フレーム開始。BeginRendering の直後に呼ぶ。</summary>
	void ImGuiManager::NewFrame()
	{
#if defined(_DEBUG) || DEV_TOOL_ENABLED
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
#endif
	}

	/// <summary>
	/// 登録された UI 関数を順に呼び出す。
	/// NewFrame() と EndFrame() の間に呼ぶこと。
	/// </summary>
	void ImGuiManager::Update()
	{
#if defined(_DEBUG) || DEV_TOOL_ENABLED
		for (auto& func : mDebugUIFunctions)
		{
			func();
		}
#endif
	}

	/// <summary>
	/// 描画データの確定と ImGui コマンドの発行。
	/// Flip() の直前に呼ぶ。
	/// </summary>
	void ImGuiManager::EndFrame()
	{
#if defined(_DEBUG) || DEV_TOOL_ENABLED
		// 描画データの確定
		ImGui::Render();

		// コマンドリストへの描画コマンド発行
		// SetDescriptorHeaps は描画直前に呼ぶ必要がある
		// （DX12 の仕様上、後から呼んだものが有効になるため）
		auto* cmdList = mRendererContext->GetCommandList();

		ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

		// マルチビューポートの更新（ViewportsEnable 時のみ）
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
#endif
	}

	/// <summary>
	/// デバッグ UI 描画関数を登録する。
	/// 登録した関数は Update() 内で毎フレーム呼ばれる。
	/// </summary>
	void ImGuiManager::AddDebugUI(std::function<void()> guiFunc)
	{
		mDebugUIFunctions.push_back(std::move(guiFunc));
	}
}
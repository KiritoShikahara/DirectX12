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
		// 【重要】この条件は NewFrame()/Update()/EndFrame() 側と必ず一致させること。
		// 以前ここだけが _DEBUG 判定で、描画側が DEV_TOOL_ENABLED 判定になっていたため、
		// Develop相当(最適化あり+開発ツール有効)の構成にすると
		// 「初期化されていないImGuiに対してNewFrame()を呼ぶ」状態になり、
		// 起動直後にアクセス違反で落ちていた。
#if !DEV_TOOL_ENABLED
		mIsInitialized = false; // 初期化フラグはfalseのまま
		return true;
#endif
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
		// （コマンドリストは呼び出し側から渡されるため DX12Context の保持は不要）
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
		mFontHeap.Release();

		mHeapManager = nullptr;
		mIsInitialized = false;

		ClearDebugUI();

		DEBUG_LOG(sys::eLogLevel::Log, "ImGuiManager finalized.");
	}

	/// <summary>フレーム開始。BeginRendering の直後に呼ぶ。</summary>
	void ImGuiManager::NewFrame()
	{
#if DEV_TOOL_ENABLED
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
#if DEV_TOOL_ENABLED
		// RemoveDebugUI が Update() 中のコールバック内から呼ばれる可能性があるため
		// （例：UI 自身の「閉じる」ボタンが RemoveDebugUI を呼ぶケース）、
		// イテレート用に keys をコピーしてから回す。
		// 実行中に mDebugUIFunctions が変更されても安全。
		std::vector<std::string> keys;
		keys.reserve(mDebugUIFunctions.size());
		for (const auto& [key, func] : mDebugUIFunctions)
			keys.push_back(key);

		for (const auto& key : keys)
		{
			auto it = mDebugUIFunctions.find(key);
			if (it != mDebugUIFunctions.end())
				it->second();
		}
#endif
	}

	/// <summary>
	/// 描画データの確定と ImGui コマンドの発行。
	///
	/// ImGui の DX12 バックエンドはスレッドセーフではないため、
	/// 必ずメインスレッドから eRenderChannel::Debug のコマンドリストを渡して呼ぶこと。
	/// </summary>
	/// <param name="cmdList">記録先のコマンドリスト</param>
	void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* cmdList)
	{
#if DEV_TOOL_ENABLED
		if (!mIsInitialized || cmdList == nullptr) return;

		// 描画データの確定
		ImGui::Render();

		// SetDescriptorHeaps はコマンドリスト単位の状態。
		// DX12Context::BeginRendering() が全チャネルに設定済みだが、
		// 同一チャネル内で他のレンダラーが差し替えている可能性があるため
		// 描画直前に張り直す。
		ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

		// マルチビューポートの更新（ViewportsEnable 時のみ）
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(nullptr, (void*)cmdList);
		}
#endif
	}

	/// <summary>
	/// デバッグ UI 描画関数を登録する。
	/// 登録した関数は Update() 内で毎フレーム呼ばれる。
	/// 同じ key で再登録すると、既存の登録を上書きする
	/// （シーン再入場時に多重登録されることを防ぐ）。
	/// </summary>
	void ImGuiManager::AddDebugUI(std::function<void()> guiFunc, const std::string& key)
	{
#if DEV_TOOL_ENABLED
		mDebugUIFunctions[key] = std::move(guiFunc);
#endif
	}

	/// <summary>
	/// key に対応するデバッグ UI 描画関数の登録を解除する。
	/// </summary>
	void ImGuiManager::RemoveDebugUI(const std::string& key)
	{
#if DEV_TOOL_ENABLED
		mDebugUIFunctions.erase(key);
#endif
	}

	/// <summary>
	/// key が現在登録されているかを確認する。
	/// </summary>
	bool ImGuiManager::HasDebugUI(const std::string& key) const
	{
#if DEV_TOOL_ENABLED
		return mDebugUIFunctions.count(key) > 0;
#else
		return false;
#endif
	}

	/// <summary>
	/// 登録されている全てのデバッグ UI を解除する。
	/// </summary>
	void ImGuiManager::ClearDebugUI()
	{
#if DEV_TOOL_ENABLED
		mDebugUIFunctions.clear();
#endif
	}
}
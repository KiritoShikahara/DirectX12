#pragma once
#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>
#include<vector>
#include<string>
#include<functional>
#include<unordered_map>


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
			sys::Window& window,
			graphics::DX12Device& device,
			graphics::DX12Context& context,
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
		/// 同じ key で再登録すると、既存の登録を上書きする。
		/// </summary>
		/// <param name="guiFunc">毎フレーム呼ばれる描画関数</param>
		/// <param name="key">登録を識別するキー（RemoveDebugUI で使用）</param>
		void AddDebugUI(std::function<void()> guiFunc, const std::string& key);

		/// <summary>
		/// key に対応するデバッグ UI 描画関数の登録を解除する。
		/// 該当する key が無い場合は何もしない（安全に呼べる）。
		/// シーンの Finalize() など、UI が不要になったタイミングで呼ぶこと。
		/// </summary>
		/// <param name="key">AddDebugUI で指定したキー</param>
		void RemoveDebugUI(const std::string& key);

		/// <summary>
		/// key が現在登録されているかを確認する。
		/// </summary>
		bool HasDebugUI(const std::string& key) const;

		/// <summary>
		/// 登録されている全てのデバッグ UI を解除する。
		/// </summary>
		void ClearDebugUI();

	private:
		/// <summary>
		/// 登録済みのデバッグ UI 描画関数。
		/// key で個別に削除できるよう unordered_map で管理する。
		/// 呼び出し順は不定になるため、表示順序に依存する UI は
		/// 呼び出し側でソートキーを key に含めるなどして調整すること。
		/// </summary>
		std::unordered_map<std::string, std::function<void()>> mDebugUIFunctions;

		/// <summary>
		/// フォント用ディスクリプタスロット
		/// </summary>
		graphics::GDescriptorHeap          mFontHeap;

		/// <summary>
		/// EndFrame() で毎フレーム使うコマンドリストの借用元。
		/// ライフタイムは Engine 側が保証する前提でポインタ保持。
		/// </summary>
		graphics::DX12Context* mRendererContext = nullptr;

		/// <summary>
		/// EndFrame() の SetDescriptorHeaps に渡すネイティブヒープの借用元。
		/// </summary>
		graphics::GDescriptorHeapManager* mHeapManager = nullptr;

		/// <summary>ImGui コンテキスト</summary>
		ImGuiContext* mContext = nullptr;

		bool                                mIsInitialized = false;
	};
}
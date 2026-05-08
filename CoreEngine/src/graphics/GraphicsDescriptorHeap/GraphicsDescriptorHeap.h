#pragma once

#include"GraphicsDescriptorHeapInfo.h"
#include<Utility/Export/Export.h>

#include<cstdint>
#include<d3d12.h>

namespace graphics
{
	class GDescriptorHeapManager;

	class ENGINE_API GDescriptorHeap
	{
	public:
		GDescriptorHeap() = default;
		~GDescriptorHeap();

		// 二重解放を防ぐためコピー禁止
		GDescriptorHeap(const GDescriptorHeap&) = delete;
		GDescriptorHeap& operator=(const GDescriptorHeap&) = delete;

		// ムーブは許可（所有権の移譲）
		GDescriptorHeap(GDescriptorHeap&&) noexcept;
		GDescriptorHeap& operator=(GDescriptorHeap&&) noexcept;
		/// <summary>
		/// スロットの確保。
		/// Manager の参照を内部に保持するため、以降のメソッド呼び出しに引数は不要。
		/// 既に確保済みの場合は一度解放してから再確保する。
		/// </summary>
		/// <param name="manager">スロットを管理するマネージャー</param>
		/// <param name="size">確保するスロット数</param>
		/// <returns>true:成功</returns>
		bool Create(GDescriptorHeapManager& manager, uint32_t size = 1);

		/// <summary>
		/// スロットの明示的な解放。デストラクタでも呼ばれる。
		/// </summary>
		void Release();

		/// <summary>CPU ハンドルの取得</summary>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const;

		/// <summary>GPU ハンドルの取得</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;

		/// <summary>有効なスロットを保持しているか</summary>
		bool IsValid() const;

		/// <summary>割り当てられたスロットのインデックス</summary>
		int GetIndex()const;

	private:
		/// <summary>
		/// スロット情報。Index == -1 のとき未確保。
		/// </summary>
		GDescriptorHeapInfo  mHeapInfo;

		/// <summary>
		/// 確保元のマネージャー。解放時に使用する。
		/// GDescriptorHeap より Manager の方が長生きする前提（Engine 管理）。
		/// </summary>
		GDescriptorHeapManager* mManager = nullptr;

	};
}
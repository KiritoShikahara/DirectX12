#pragma once

#include "GraphicsDescriptorHeapInfo.h"
#include <Utility/Export/Export.h>
#include <cstdint>
#include <d3d12.h>

namespace graphics
{
	class GDescriptorHeapManager;

	class ENGINE_API GDescriptorHeap
	{
	public:
		GDescriptorHeap() = default;
		~GDescriptorHeap();

		GDescriptorHeap(const GDescriptorHeap&) = delete;
		GDescriptorHeap& operator=(const GDescriptorHeap&) = delete;

		GDescriptorHeap(GDescriptorHeap&& other) noexcept;
		GDescriptorHeap& operator=(GDescriptorHeap&& other) noexcept;

		/// <summary>
		/// スロットの確保
		/// </summary>
		bool Create(GDescriptorHeapManager& manager, uint32_t size = 1);

		/// <summary>
		/// スロットの明示的な解放
		/// </summary>
		void Release();

		/// <summary>
		/// CPU ハンドルの取得
		/// </summary>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const;

		/// <summary>
		/// GPU ハンドルの取得
		/// </summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;

		/// <summary>
		/// 有効なスロットを保持しているか
		/// </summary>
		bool IsValid() const;

		/// <summary>
		/// 割り当てられたスロットのインデックス
		/// </summary>
		int GetIndex() const;

	private:
		/// <summary>
		/// スロット情報
		/// </summary>
		GDescriptorHeapInfo  mHeapInfo;

		/// <summary>
		/// 確保元のマネージャー
		/// </summary>
		GDescriptorHeapManager* mManager = nullptr;
	};
}
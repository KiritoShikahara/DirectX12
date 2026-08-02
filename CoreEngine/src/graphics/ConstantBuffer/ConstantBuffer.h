#pragma once

#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>
#include<graphics/Dx12/Dx12Type.h>
#include<Utility/Export/Export.h>

#include<cstdint>

namespace graphics
{
	class DX12Device;
	class GDescriptorHeapManager;

	/// <summary>
	/// 定数バッファ。
	/// D3D12MA でバッファを確保し、GDescriptorHeapManager から CBV スロットを取得して登録する。
	/// GetGpuHandle() でディスクリプタテーブルにバインドして使う。
	/// </summary>
	class ENGINE_API ConstantBuffer
	{
        /// <summary>現在のフレームインデックス</summary>
        uint32_t GetCurrentIndex() const;
	public:
		ConstantBuffer() = default;
		~ConstantBuffer() = default;

		ConstantBuffer(const ConstantBuffer&) = delete;
		ConstantBuffer& operator=(const ConstantBuffer&) = delete;
		ConstantBuffer(ConstantBuffer&&) = default;
		ConstantBuffer& operator=(ConstantBuffer&&) = default;

        /// <summary>
        /// バッファと CBV ディスクリプタを作成する。
        /// byteSize は自動で 256 byte アライメントされる。
        /// </summary>
        bool Create(
            DX12Device& device,
            GDescriptorHeapManager& heapManager,
            uint32_t                byteSize);

        /// <summary>CPU からデータを書き込む（Upload heap 永続マップ）。</summary>
        void Update(const void* data, uint32_t byteSize);

        /// <summary>
        /// GPU ハンドル（SetGraphicsRootDescriptorTable に渡す）。
        /// </summary>
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;

        bool IsValid() const;

    private:
        /// <summary>
		/// 1フレーム分のリソース情報。
        /// </summary>
        struct FrameResource
        {
            Resource        Resource = nullptr;
            MAAllocation    Alloc = nullptr;
            void* MappedPtr = nullptr;
            GDescriptorHeap CbvHeap;
        };
        std::array<FrameResource, FRAME_COUNT> mFrames;

        uint32_t mAlignedSize = 0;
	};
}



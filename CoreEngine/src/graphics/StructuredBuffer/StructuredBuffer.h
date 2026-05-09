#pragma once
#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapInfo.h>

#include<vector>

namespace graphics
{
	class ENGINE_API StructuredBuffer
	{
		/// <summary>
		/// 現在のインデックスを取得するヘルパー
		/// dx12から取得する
		/// </summary>
		/// <returns></returns>
		uint32_t GetCurrentIndex() const;
	public:
		StructuredBuffer();
		virtual ~StructuredBuffer();

		// リソース持ってるのでコピーダメ
		StructuredBuffer(const StructuredBuffer&) = delete;
		StructuredBuffer& operator=(const StructuredBuffer&) = delete;

        /// <summary>
        /// 構造化バッファの作成
        /// </summary>
        /// <param name="ElementSize">構造体サイズ</param>
        /// <param name="ElementCount">数</param>
        /// <returns></returns>
        bool Create(size_t ElementSize, uint32_t ElementCount);

        /// <summary>
        /// リソース解放
        /// </summary>
        void Release();

        /// <summary>
        /// データをGPUに転送
        /// </summary>
        /// <param name="SrcData"></param>
        /// <param name="Size"></param>
        void Update(const void* SrcData, size_t Size);

        /// <summary>
        /// DescriptorTable用のGPUハンドル取得 
        /// </summary>
        /// <returns></returns>
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;

	private:
        /// <summary>
        /// フレームごとのリソース
        /// </summary>
        struct FrameResource {
            Resource Resource = nullptr;
            MAAllocation Allocation = nullptr;
            GDescriptorHeapInfo HeapInfo;
            void* MappedData = nullptr;
        };
        std::vector<FrameResource> mFrames;

        /// <summary>
        /// １つ単位のサイズ
        /// </summary>
        size_t mElementSize;

        /// <summary>
        /// 数
        /// </summary>
        uint32_t mElementCount;

        /// <summary>
        /// 全てのサイズ
        /// </summary>
        size_t mTotalSize;

	};
}



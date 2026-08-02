#include"pch.h"
#include "StructuredBuffer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Dx12/RenderContext.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>

namespace graphics
{
	uint32_t StructuredBuffer::GetCurrentIndex() const
	{
		return graphics::RenderContext::Get().GetFrameIndex();
	}

	StructuredBuffer::StructuredBuffer()
		:mElementSize(0)
		, mElementCount(0)
		, mTotalSize(0)
	{
	}

	StructuredBuffer::~StructuredBuffer()
	{
		Release();
	}

	/// <summary>
	/// 構造化バッファの作成
	/// </summary>
	/// <param name="ElementSize">構造体サイズ</param>
	/// <param name="ElementCount">数</param>
	/// <returns></returns>
	bool StructuredBuffer::Create(size_t ElementSize, uint32_t ElementCount)
	{
		Release();

		auto& d3d12Device = graphics::DX12Device::Get();
		auto allocator = d3d12Device.GetMAAllocator();
		auto device = d3d12Device.GetDevice();
		auto& heapManager = graphics::GDescriptorHeapManager::Get();

		mElementSize = ElementSize;
		mElementCount = ElementCount;
		mTotalSize = ElementSize * ElementCount;

		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(mTotalSize);
		D3D12MA::ALLOCATION_DESC allocDesc = { .HeapType = D3D12_HEAP_TYPE_UPLOAD };

		uint32_t frameCount = FRAME_COUNT;
		mFrames.resize(frameCount);
		for (uint32_t i = 0; i < frameCount; ++i) {
			// リソース作成
			allocator->CreateResource(&allocDesc, &resDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				&mFrames[i].Allocation, IID_PPV_ARGS(&mFrames[i].Resource));

			mFrames[i].Resource->Map(0, nullptr, &mFrames[i].MappedData);

			// 自前ヒープを作らず、マネージャーから1スロット確保
			mFrames[i].HeapInfo = heapManager.Issuance(1);
			if (!mFrames[i].HeapInfo.IsValid()) return false;

			// SRV設定
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.NumElements = ElementCount;
			srvDesc.Buffer.StructureByteStride = (UINT)ElementSize;

			// マネージャーが管理する共通ヒープの場所にSRVを書き込む
			device->CreateShaderResourceView(
				mFrames[i].Resource.Get(),
				&srvDesc,
				heapManager.GetCpuHandle(mFrames[i].HeapInfo)
			);
		}
		return true;
	}

	/// <summary>
	/// リソース解放
	/// </summary>
	void StructuredBuffer::Release()
	{
		auto& heapManager = graphics::GDescriptorHeapManager::Get();
		for (auto& frame : mFrames) {
			if (frame.Resource && frame.MappedData) {
				frame.Resource->Unmap(0, nullptr);
			}
			// スロットを返す
			if (frame.HeapInfo.IsValid()) {
				heapManager.Discard(frame.HeapInfo);
			}
			frame.Allocation.Reset();
			frame.Resource.Reset();

		}
		mFrames.clear();
		mElementSize = 0;
		mElementCount = 0;
		mTotalSize = 0;
	}

	/// <summary>
	/// データをGPUに転送
	/// </summary>
	/// <param name="SrcData"></param>
	/// <param name="Size"></param>
	void StructuredBuffer::Update(const void* SrcData, size_t Size)
	{
		if (!SrcData || mFrames.empty()) return;

		// 今のフレームのリソース取得
		auto& frame = mFrames[GetCurrentIndex()];

		// サイズが大きすぎないか
		size_t copySize = (Size < mTotalSize) ? Size : mTotalSize;
		memcpy(frame.MappedData, SrcData, copySize);

	}

	/// <summary>
	/// DescriptorTable用のGPUハンドル取得 
	/// </summary>
	/// <returns></returns>
	D3D12_GPU_DESCRIPTOR_HANDLE StructuredBuffer::GetGpuHandle()const
	{
		auto& frame = mFrames[GetCurrentIndex()];
		return GDescriptorHeapManager::Get().GetGpuHandle(frame.HeapInfo);
	}

}
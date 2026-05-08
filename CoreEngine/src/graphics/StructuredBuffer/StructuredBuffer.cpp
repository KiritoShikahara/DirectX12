#include"pch.h"
#include "StructuredBuffer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Dx12/RenderContext.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>

namespace graphics
{
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




		return true;
	}



	/// <summary>
	/// 現在のインデックスを取得するヘルパー
	/// </summary>
	/// <returns></returns>
	uint32_t StructuredBuffer::GetCurrentIndex() const
	{
		return graphics::RenderContext::Get().GetFrameIndex();
	}
}
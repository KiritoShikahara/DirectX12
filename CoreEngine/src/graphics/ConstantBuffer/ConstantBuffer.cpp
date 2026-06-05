#include"pch.h"
#include "ConstantBuffer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>

namespace graphics
{
    bool ConstantBuffer::Create(
        DX12Device& device,
        GDescriptorHeapManager& heapManager,
        uint32_t                byteSize)
    {
        // CBV は 256 byte アライメント
        const uint32_t alignedSize = (byteSize + 255) & ~255u;

        // D3D12MA で Upload heap にバッファを確保する
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);

        HRESULT hr = device.GetMAAllocator()->CreateResource(
            &allocDesc,
            &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            &mAlloc,
            IID_PPV_ARGS(&mResource));

        if (FAILED(hr))
        {
			DEBUG_LOG(sys::eLogLevel::Error, "Failed to create ConstantBuffer resource.");
            return false;
        }

		// 永続マッピング
        hr = mResource->Map(0, nullptr, &mMappedPtr);
        if (FAILED(hr))
        {
			DEBUG_LOG(sys::eLogLevel::Error, "Failed to map ConstantBuffer resource.");
            return false;
        }

		// GDescriptorHeapManager から CBV スロットを確保して登録する
        if (!mCbvHeap.Create(heapManager, 1))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "ConstantBuffer: Failed to allocate CBV descriptor slot.");
            return false;
        }

        // 取得したスロットの CPU ハンドルに CBV を登録する
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = mResource->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = alignedSize;

        device.GetDevice()->CreateConstantBufferView(&cbvDesc, mCbvHeap.GetCpuHandle());

        return true;
    }

    void ConstantBuffer::Update(const void* data, uint32_t byteSize)
    {
        if (mMappedPtr == nullptr) return;
        memcpy(mMappedPtr, data, byteSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE ConstantBuffer::GetGpuHandle() const
    {
        return mCbvHeap.GetGpuHandle();
    }
}
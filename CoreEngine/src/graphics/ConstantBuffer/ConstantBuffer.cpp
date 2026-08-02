#include"pch.h"
#include "ConstantBuffer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include<graphics/Dx12/RenderContext.h>

namespace graphics
{

    uint32_t ConstantBuffer::GetCurrentIndex() const
    {
        return graphics::RenderContext::Get().GetFrameIndex();
    }

    bool ConstantBuffer::Create(
        DX12Device& device, GDescriptorHeapManager& heapManager, uint32_t byteSize)
    {
        mAlignedSize = (byteSize + 255) & ~255u;

        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(mAlignedSize);

        for (uint32_t i = 0; i < FRAME_COUNT; ++i)
        {
            auto& f = mFrames[i];

            HRESULT hr = device.GetMAAllocator()->CreateResource(
                &allocDesc, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, &f.Alloc, IID_PPV_ARGS(&f.Resource));
            if (FAILED(hr))
            {
                DEBUG_LOG(sys::eLogLevel::Error, "ConstantBuffer: Failed to create resource.");
                return false;
            }

            hr = f.Resource->Map(0, nullptr, &f.MappedPtr);
            if (FAILED(hr))
            {
                DEBUG_LOG(sys::eLogLevel::Error, "ConstantBuffer: Failed to map resource.");
                return false;
            }

            if (!f.CbvHeap.Create(heapManager, 1))
            {
                DEBUG_LOG(sys::eLogLevel::Error, "ConstantBuffer: Failed to allocate CBV slot.");
                return false;
            }

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = f.Resource->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = mAlignedSize;
            device.GetDevice()->CreateConstantBufferView(&cbvDesc, f.CbvHeap.GetCpuHandle());
        }
        return true;
    }

    void ConstantBuffer::Update(const void* data, uint32_t byteSize)
    {
        auto& f = mFrames[GetCurrentIndex()];
        if (f.MappedPtr == nullptr) return;
        memcpy(f.MappedPtr, data, (byteSize < mAlignedSize) ? byteSize : mAlignedSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE ConstantBuffer::GetGpuHandle() const
    {
        return mFrames[GetCurrentIndex()].CbvHeap.GetGpuHandle();
    }

    bool ConstantBuffer::IsValid() const
    {
        const auto& f = mFrames[GetCurrentIndex()];
        return f.CbvHeap.IsValid() && f.MappedPtr != nullptr;
    }
}
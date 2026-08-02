#include "pch.h"
#include "IndexBuffer.h"
#include <graphics/Dx12/Dx12Device.h>

namespace graphics
{
	IndexBuffer::IndexBuffer()
		: mBufferView({})
		, mBufferResource(nullptr)
		, mUploadResource(nullptr)
		, mBufferSize(0)
		, mFormat(DXGI_FORMAT_R32_UINT)
		, mMapped(nullptr)
	{
	}

	IndexBuffer::~IndexBuffer()
	{
		Release();
	}

	bool IndexBuffer::CreateDynamic(const size_t Size, const DXGI_FORMAT Format)
	{
		mBufferSize = Size;
		mFormat = Format;
		mIsDynamic = true;

		auto device = graphics::DX12Device::Get().GetDevice();

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferSize);

		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mBufferResource)
		);
		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Failed to create dynamic IndexBuffer.");
			return false;
		}

		// 永続マッピングの開始
		CD3DX12_RANGE readRange(0, 0);
		mBufferResource->Map(0, &readRange, &mMapped);

		// ビューのキャッシュ
		mBufferView.BufferLocation = mBufferResource->GetGPUVirtualAddress();
		mBufferView.SizeInBytes = static_cast<UINT>(mBufferSize);
		mBufferView.Format = mFormat;

		return true;
	}

	bool IndexBuffer::CreateStatic(ID3D12GraphicsCommandList* CmdList, const void* InitData, const size_t Size, const DXGI_FORMAT Format)
	{
		mBufferSize = Size;
		mFormat = Format;
		mIsDynamic = false;

		auto device = graphics::DX12Device::Get().GetDevice();
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferSize);

		// Default ヒープにリソースを作成
		auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		HRESULT hr = device->CreateCommittedResource(
			&defaultHeap,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&mBufferResource)
		);
		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Failed to create static IndexBuffer (default heap).");
			return false;
		}

		// Upload ヒープに一時リソースを作成
		auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		hr = device->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadResource)
		);
		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Failed to create static IndexBuffer (upload heap).");
			return false;
		}

		// 一時リソースへデータを書き込む
		void* mapped = nullptr;
		mUploadResource->Map(0, nullptr, &mapped);
		memcpy(mapped, InitData, Size);
		mUploadResource->Unmap(0, nullptr);

		// GPU 側でコピー
		CmdList->CopyBufferRegion(mBufferResource.Get(), 0, mUploadResource.Get(), 0, Size);

		// バッファ状態の遷移
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			mBufferResource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_INDEX_BUFFER
		);
		CmdList->ResourceBarrier(1, &barrier);

		// ビューのキャッシュ
		mBufferView.BufferLocation = mBufferResource->GetGPUVirtualAddress();
		mBufferView.SizeInBytes = static_cast<UINT>(mBufferSize);
		mBufferView.Format = mFormat;

		return true;
	}

	bool IndexBuffer::CreateStaticSync(const void* InitData, size_t Size, DXGI_FORMAT Format)
	{
		if (!InitData || Size == 0)
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"IndexBuffer::CreateStaticSync: Invalid arguments (nullptr or size=0).");
			return false;
		}
		if (Format != DXGI_FORMAT_R16_UINT && Format != DXGI_FORMAT_R32_UINT)
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"IndexBuffer::CreateStaticSync: Invalid format. Use R16_UINT or R32_UINT.");
			return false;
		}

		mBufferSize = Size;
		mFormat = Format;
		mIsDynamic = false;

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferSize);

		HRESULT hr = graphics::DX12Device::Get().GetMAAllocator()->CreateResource(
			&allocDesc,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			&mBufferAllocation,
			IID_PPV_ARGS(&mBufferResource));

		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"IndexBuffer::CreateStaticSync: Failed to create DEFAULT heap resource (D3D12MA).");
			return false;
		}

		if (!graphics::DX12Device::Get().UploadBufferData(
			mBufferResource.Get(), InitData, Size,
			D3D12_RESOURCE_STATE_INDEX_BUFFER))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"IndexBuffer::CreateStaticSync: UploadBufferData failed.");
			mBufferAllocation.Reset();
			mBufferResource.Reset();
			return false;
		}

		// ビューのキャッシュ
		mBufferView.BufferLocation = mBufferResource->GetGPUVirtualAddress();
		mBufferView.SizeInBytes = static_cast<UINT>(mBufferSize);
		mBufferView.Format = mFormat;

		return true;
	}

	void IndexBuffer::Release()
	{
		if (mBufferAllocation != nullptr)
		{
			mBufferAllocation.Reset();
		}

		if (mBufferResource != nullptr)
		{
			if (mIsDynamic && mMapped != nullptr)
			{
				mBufferResource->Unmap(0, nullptr);
				mMapped = nullptr;
			}
			mBufferResource.Reset();
		}
	}

	void IndexBuffer::ReleaseUploadBuffer()
	{
		mUploadResource.Reset();
	}

	void IndexBuffer::Update(const void* SrcData, const size_t Size, const size_t Offset)
	{
		if (!mIsDynamic)
		{
			DEBUG_LOG(sys::eLogLevel::Warning, "IndexBuffer::Update called on a static buffer.");
			return;
		}
		if (mMapped == nullptr || (Offset + Size) > mBufferSize)
		{
			DEBUG_LOG(sys::eLogLevel::Warning, "IndexBuffer update out of range or not initialized.");
			return;
		}

		memcpy(static_cast<uint8_t*>(mMapped) + Offset, SrcData, Size);
	}

	void IndexBuffer::Set(ID3D12GraphicsCommandList* CmdList) const
	{
		if (CmdList == nullptr) return;
		CmdList->IASetIndexBuffer(&mBufferView);
	}

	size_t IndexBuffer::GetSize() const
	{
		return mBufferSize;
	}

	uint32_t IndexBuffer::GetCount() const
	{
		if (mBufferSize == 0) return 0;
		const size_t indexByteSize = (mFormat == DXGI_FORMAT_R16_UINT) ? sizeof(uint16_t) : sizeof(uint32_t);
		return static_cast<uint32_t>(mBufferSize / indexByteSize);
	}

	DXGI_FORMAT IndexBuffer::GetFormat() const
	{
		return mFormat;
	}

}
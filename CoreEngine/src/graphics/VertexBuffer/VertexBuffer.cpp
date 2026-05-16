#include"pch.h"
#include "VertexBuffer.h"

#include<graphics/Dx12/Dx12Device.h>

namespace graphics
{
	VertexBuffer::VertexBuffer()
		:mBufferView({})
		, mBufferResource(nullptr)
		, mUploadResource(nullptr)
		, mBufferSize(0)
		, mStride(0)
		, mMapped(nullptr)
	{
	}

	VertexBuffer::~VertexBuffer()
	{
		Release();
	}

	/// <summary>
	/// 頂点バッファの作成
	/// </summary>
	/// <param name="size">バッファのサイズ</param>
	/// <param name="stride">1頂点のデータサイズ</param>
	/// <returns>true:成功</returns>
	bool VertexBuffer::CreateDynamic(const size_t Size, const size_t Stride)
	{

		mBufferSize = Size;
		mStride = Stride;
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
			DEBUG_LOG(sys::eLogLevel::Error, "Failed to create VertexBuffer.");
			return false;
		}

		// 永続マッピングの開始
		CD3DX12_RANGE readRange(0, 0);
		mBufferResource->Map(0, &readRange, &mMapped);

		// ビューのキャッシュ
		mBufferView.BufferLocation = mBufferResource->GetGPUVirtualAddress();
		mBufferView.SizeInBytes = static_cast<UINT>(mBufferSize);
		mBufferView.StrideInBytes = static_cast<UINT>(mStride);

		// フラグを立て
		mIsDynamic = true;


		return true;
	}

	/// <summary>
	/// 静的な頂点バッファの作成
	/// </summary>
	/// <returns></returns>
	bool VertexBuffer::CreateStatic(ID3D12GraphicsCommandList* CmdList, const void* InitData, const size_t Size, const size_t Stride)
	{
		mBufferSize = Size;
		mStride = Stride;
		mIsDynamic = false;
		auto device = graphics::DX12Device::Get().GetDevice();
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferSize);

		// Default ヒープにリソースを作成
		auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		HRESULT hr = device->CreateCommittedResource(
			&defaultHeap,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, // ★最初はコピー先として作成
			nullptr,
			IID_PPV_ARGS(&mBufferResource)
		);
		if (FAILED(hr)) return false;

		// CPUから書き込める Upload ヒープに一時リソースを作成
		auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		hr = device->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadResource)
		);
		if (FAILED(hr)) return false;

		// 一時リソースにデータを書き込む
		void* mapped = nullptr;
		mUploadResource->Map(0, nullptr, &mapped);
		memcpy(mapped, InitData, Size);
		mUploadResource->Unmap(0, nullptr);

		// GPU上で Upload -> Default へデータをコピーするコマンドを積む
		CmdList->CopyBufferRegion(mBufferResource.Get(), 0, mUploadResource.Get(), 0, Size);

		// バッファの状態を「コピー先」から「頂点バッファとして読み取り可能」に変更
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			mBufferResource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		);
		CmdList->ResourceBarrier(1, &barrier);

		// ビューのキャッシュ
		mBufferView.BufferLocation = mBufferResource->GetGPUVirtualAddress();
		mBufferView.SizeInBytes = static_cast<UINT>(mBufferSize);
		mBufferView.StrideInBytes = static_cast<UINT>(mStride);

		return true;
	}

	/// <summary>
	/// バッファの解放
	/// </summary>
	void VertexBuffer::Release()
	{
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

	void VertexBuffer::ReleaseUploadBuffer()
	{
		mUploadResource.Reset();
	}

	/// <summary>
	/// CPU上のデータをバッファへ転送する
	/// </summary>
	/// <param name="SrcData">Map/Unmapを介さない永続マッピングポインタへのmemcpyにより、最小限のコストで更新します</param>
	/// <param name="Size">転送するサイズ</param>
	/// <param name="Offset">バッファ先頭からの書き込みオフセット</param>
	void VertexBuffer::Update(const void* SrcData, size_t Size, size_t Offset)
	{
		if (!mIsDynamic)
		{
			DEBUG_LOG(sys::eLogLevel::Warning, "Attempting to update a static VertexBuffer. This operation is not allowed.");
			return;
		}

		if (mMapped == nullptr || (Offset + Size) > mBufferSize)
		{
			DEBUG_LOG(sys::eLogLevel::Warning, "VertexBuffer update out of range or not initialized.");
			return;
		}

		memcpy(static_cast<uint8_t*>(mMapped) + Offset, SrcData, Size);
	}

	/// <summary>
	/// 頂点バッファビューをコマンドリストにセット
	/// </summary>
	/// <param name="CmdList">描画命令を記録するコマンドリスト</param>
	/// <param name="Slot">セットする入力スロットのインデックス</param>
	void VertexBuffer::Set(ID3D12GraphicsCommandList* CmdList, uint32_t Slot) const
	{
		if (CmdList == nullptr) return;
		// 頂点バッファは配列形式で渡す必要がある
		CmdList->IASetVertexBuffers(Slot, 1, &mBufferView);
	}

	/// <summary>
	/// バッファの一部範囲や、異なるストライドとしてビューをセット
	/// </summary>
	/// <param name="CmdList"></param>
	/// <param name="Offset">ビューの開始地点オフセット</param>
	/// <param name="Size">ビューの対象サイズ</param>
	/// <param name="Stride">1頂点あたりのサイズ</param>
	/// <param name="Slot">入力スロット</param>
	void VertexBuffer::Set(ID3D12GraphicsCommandList* CmdList, size_t Offset, size_t Size, size_t Stride, uint32_t Slot) const
	{
		if (CmdList == nullptr) return;

		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = mBufferView.BufferLocation + Offset;
		vbv.SizeInBytes = static_cast<UINT>(Size);
		vbv.StrideInBytes = static_cast<UINT>(Stride);

		CmdList->IASetVertexBuffers(Slot, 1, &vbv);
	}

	/// <summary>
	/// 確保済みバッファの全バイトサイズ取得
	/// </summary>
	/// <returns></returns>
	size_t VertexBuffer::GetSize()const
	{
		return mBufferSize;
	}

	/// <summary>
	/// 1頂点あたりのバイトサイズを取得
	/// </summary>
	/// <returns></returns>
	size_t VertexBuffer::GetStride() const
	{
		return mStride;
	}

}

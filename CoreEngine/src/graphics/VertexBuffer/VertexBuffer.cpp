#include"pch.h"
#include "VertexBuffer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Dx12/RenderContext.h>

namespace graphics
{
	/// <summary>
	/// 現在のフレームインデックスを取得するヘルパー。
	/// RenderContext から取得する。
	/// </summary>
	uint32_t VertexBuffer::GetCurrentIndex() const
	{
		return graphics::RenderContext::Get().GetFrameIndex();
	}

	VertexBuffer::VertexBuffer()
		: mBufferView({})
		, mBufferAllocation(nullptr)
		, mBufferResource(nullptr)
		, mUploadResource(nullptr)
		, mBufferSize(0)
		, mStride(0)
		, mIsDynamic(true)
	{
	}

	VertexBuffer::~VertexBuffer()
	{
		Release();
	}

	/// <summary>
	/// 動的頂点バッファの作成。
	/// FRAME_COUNT 個の UPLOAD リソースを確保して永続マップする。
	/// </summary>
	/// <param name="Size">バッファ 1 個あたりのサイズ</param>
	/// <param name="Stride">1 頂点のデータサイズ</param>
	/// <returns>true:成功</returns>
	bool VertexBuffer::CreateDynamic(const size_t Size, const size_t Stride)
	{
		if (Size == 0 || Stride == 0)
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"VertexBuffer::CreateDynamic: Invalid arguments (size=0 or stride=0).");
			return false;
		}

		Release();

		mBufferSize = Size;
		mStride = Stride;
		mIsDynamic = true;

		auto* allocator = graphics::DX12Device::Get().GetMAAllocator();

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferSize);

		for (uint32_t i = 0; i < FRAME_COUNT; ++i)
		{
			auto& frame = mDynamicFrames[i];

			HRESULT hr = allocator->CreateResource(
				&allocDesc,
				&resDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				&frame.Allocation,
				IID_PPV_ARGS(&frame.BufferResource));

			if (FAILED(hr))
			{
				DEBUG_LOG(sys::eLogLevel::Error,
					"VertexBuffer::CreateDynamic: Failed to create UPLOAD resource.");
				Release();
				return false;
			}

			// 永続マッピングの開始（CPU からは読まないので readRange は空）
			CD3DX12_RANGE readRange(0, 0);
			hr = frame.BufferResource->Map(0, &readRange, &frame.Mapped);
			if (FAILED(hr))
			{
				DEBUG_LOG(sys::eLogLevel::Error,
					"VertexBuffer::CreateDynamic: Failed to map UPLOAD resource.");
				Release();
				return false;
			}

			// ビューのキャッシュ
			frame.View.BufferLocation = frame.BufferResource->GetGPUVirtualAddress();
			frame.View.SizeInBytes = static_cast<UINT>(mBufferSize);
			frame.View.StrideInBytes = static_cast<UINT>(mStride);
		}

		return true;
	}

	/// <summary>
	/// 静的な頂点バッファの作成（コマンドリスト経由）
	/// </summary>
	/// <returns>true:成功</returns>
	bool VertexBuffer::CreateStatic(ID3D12GraphicsCommandList* CmdList, const void* InitData, const size_t Size, const size_t Stride)
	{
		if (CmdList == nullptr || InitData == nullptr || Size == 0 || Stride == 0)
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"VertexBuffer::CreateStatic: Invalid arguments.");
			return false;
		}

		Release();

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
			D3D12_RESOURCE_STATE_COPY_DEST, // 最初はコピー先として作成
			nullptr,
			IID_PPV_ARGS(&mBufferResource));
		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"VertexBuffer::CreateStatic: Failed to create DEFAULT heap resource.");
			return false;
		}

		// CPU から書き込むための Upload ヒープに一時リソースを作成
		auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		hr = device->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadResource));
		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"VertexBuffer::CreateStatic: Failed to create UPLOAD heap resource.");
			return false;
		}

		// 一時リソースにデータを書き込む
		void* mapped = nullptr;
		mUploadResource->Map(0, nullptr, &mapped);
		memcpy(mapped, InitData, Size);
		mUploadResource->Unmap(0, nullptr);

		// GPU 上で Upload -> Default へデータをコピーするコマンドを積む
		CmdList->CopyBufferRegion(mBufferResource.Get(), 0, mUploadResource.Get(), 0, Size);

		// バッファの状態を「コピー先」から「頂点バッファとして読み取り可能」に変更
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			mBufferResource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		CmdList->ResourceBarrier(1, &barrier);

		// ビューのキャッシュ
		mBufferView.BufferLocation = mBufferResource->GetGPUVirtualAddress();
		mBufferView.SizeInBytes = static_cast<UINT>(mBufferSize);
		mBufferView.StrideInBytes = static_cast<UINT>(mStride);

		return true;
	}

	/// <summary>
	/// 同期的な静的頂点バッファの作成。
	/// DX12Device の専用アップロードキューを使うため cmdList 不要。
	/// </summary>
	bool VertexBuffer::CreateStaticSync(const void* InitData, size_t Size, size_t Stride)
	{
		// ガード
		if (!InitData || Size == 0 || Stride == 0)
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"VertexBuffer::CreateStaticSync: Invalid arguments (nullptr or size=0).");
			return false;
		}

		Release();

		mBufferSize = Size;
		mStride = Stride;
		mIsDynamic = false;

		// DEFAULT ヒープに頂点バッファを D3D12MA で確保
		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferSize);

		HRESULT hr = graphics::DX12Device::Get().GetMAAllocator()->CreateResource(
			&allocDesc,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,   // UploadBufferData は COPY_DEST を期待する
			nullptr,
			&mBufferAllocation,
			IID_PPV_ARGS(&mBufferResource));

		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"VertexBuffer::CreateStaticSync: Failed to create DEFAULT heap resource (D3D12MA).");
			return false;
		}

		if (!graphics::DX12Device::Get().UploadBufferData(
			mBufferResource.Get(), InitData, Size,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER))
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"VertexBuffer::CreateStaticSync: UploadBufferData failed.");
			mBufferAllocation.Reset();
			mBufferResource.Reset();
			return false;
		}

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
		// ---- Dynamic ----
		// Unmap -> Resource -> Allocation の順で解放する
		for (auto& frame : mDynamicFrames)
		{
			if (frame.BufferResource != nullptr && frame.Mapped != nullptr)
			{
				frame.BufferResource->Unmap(0, nullptr);
			}
			frame.Mapped = nullptr;
			frame.BufferResource.Reset();
			frame.Allocation.Reset();
			frame.View = {};
		}

		// ---- Static ----
		mUploadResource.Reset();
		mBufferResource.Reset();
		mBufferAllocation.Reset();
		mBufferView = {};

		mBufferSize = 0;
		mStride = 0;
	}

	/// <summary>
	/// アップロード用の一時バッファを解放する
	/// </summary>
	void VertexBuffer::ReleaseUploadBuffer()
	{
		mUploadResource.Reset();
	}

	/// <summary>
	/// CPU 上のデータを現在フレームのバッファへ転送する
	/// </summary>
	/// <param name="SrcData">転送元データ</param>
	/// <param name="Size">転送するサイズ</param>
	/// <param name="Offset">バッファ先頭からの書き込みオフセット</param>
	void VertexBuffer::Update(const void* SrcData, size_t Size, size_t Offset)
	{
		if (!mIsDynamic)
		{
			DEBUG_LOG(sys::eLogLevel::Warning,
				"Attempting to update a static VertexBuffer. This operation is not allowed.");
			return;
		}

		if (SrcData == nullptr) return;

		if ((Offset + Size) > mBufferSize)
		{
			DEBUG_LOG(sys::eLogLevel::Warning, "VertexBuffer update out of range.");
			return;
		}

		auto& frame = mDynamicFrames[GetCurrentIndex()];
		if (frame.Mapped == nullptr)
		{
			DEBUG_LOG(sys::eLogLevel::Warning, "VertexBuffer is not initialized.");
			return;
		}

		memcpy(static_cast<uint8_t*>(frame.Mapped) + Offset, SrcData, Size);
	}

	/// <summary>
	/// 全フレーム分のバッファへ同一データを書き込む
	/// </summary>
	void VertexBuffer::UpdateAll(const void* SrcData, size_t Size, size_t Offset)
	{
		if (!mIsDynamic)
		{
			DEBUG_LOG(sys::eLogLevel::Warning,
				"Attempting to update a static VertexBuffer. This operation is not allowed.");
			return;
		}

		if (SrcData == nullptr) return;

		if ((Offset + Size) > mBufferSize)
		{
			DEBUG_LOG(sys::eLogLevel::Warning, "VertexBuffer update out of range.");
			return;
		}

		for (auto& frame : mDynamicFrames)
		{
			if (frame.Mapped == nullptr) continue;
			memcpy(static_cast<uint8_t*>(frame.Mapped) + Offset, SrcData, Size);
		}
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
		const D3D12_VERTEX_BUFFER_VIEW& view = GetView();
		CmdList->IASetVertexBuffers(Slot, 1, &view);
	}

	/// <summary>
	/// バッファの一部範囲を、異なるストライドとしてビューをセット
	/// </summary>
	/// <param name="CmdList">描画命令を記録するコマンドリスト</param>
	/// <param name="Offset">ビューの開始地点オフセット</param>
	/// <param name="Size">ビューの対象サイズ</param>
	/// <param name="Stride">1 頂点あたりのサイズ</param>
	/// <param name="Slot">入力スロット</param>
	void VertexBuffer::Set(ID3D12GraphicsCommandList* CmdList, size_t Offset, size_t Size, size_t Stride, uint32_t Slot) const
	{
		if (CmdList == nullptr) return;

		const D3D12_VERTEX_BUFFER_VIEW& base = GetView();

		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = base.BufferLocation + Offset;
		vbv.SizeInBytes = static_cast<UINT>(Size);
		vbv.StrideInBytes = static_cast<UINT>(Stride);

		CmdList->IASetVertexBuffers(Slot, 1, &vbv);
	}

	/// <summary>
	/// 確保済みバッファの全バイトサイズ取得
	/// </summary>
	size_t VertexBuffer::GetSize() const
	{
		return mBufferSize;
	}

	/// <summary>
	/// 1 頂点あたりのバイトサイズを取得
	/// </summary>
	size_t VertexBuffer::GetStride() const
	{
		return mStride;
	}

	/// <summary>
	/// 現在フレームの頂点バッファビューを取得
	/// </summary>
	const D3D12_VERTEX_BUFFER_VIEW& VertexBuffer::GetView() const
	{
		return mIsDynamic
			? mDynamicFrames[GetCurrentIndex()].View
			: mBufferView;
	}

	/// <summary>
	/// 有効なバッファかどうか
	/// </summary>
	bool VertexBuffer::IsValid() const
	{
		return mIsDynamic
			? (mDynamicFrames[GetCurrentIndex()].BufferResource != nullptr)
			: (mBufferResource != nullptr);
	}
}
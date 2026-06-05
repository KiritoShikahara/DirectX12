#include"pch.h"
#include "IndexBuffer.h"

#include<graphics/Dx12/Dx12Device.h>

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

	/// <summary>
	/// 動的インデックスバッファの作成
	/// Upload ヒープに配置し、永続マッピングで CPU から毎フレーム更新できる
	/// </summary>
	/// <param name="Size">バッファのバイトサイズ</param>
	/// <param name="Format">DXGI_FORMAT_R16_UINT または DXGI_FORMAT_R32_UINT</param>
	/// <returns>true: 成功</returns>
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

		// 永続マッピングの開始（CPU 書き込みのみ、GPU 読み取り範囲は指定しない）
		CD3DX12_RANGE readRange(0, 0);
		mBufferResource->Map(0, &readRange, &mMapped);

		// ビューのキャッシュ
		mBufferView.BufferLocation = mBufferResource->GetGPUVirtualAddress();
		mBufferView.SizeInBytes = static_cast<UINT>(mBufferSize);
		mBufferView.Format = mFormat;

		return true;
	}

	/// <summary>
/// 静的インデックスバッファの作成
/// Default ヒープ（GPU 専用）へ転送し、以後 CPU からは書き込めない
/// GPU 転送完了後に ReleaseUploadBuffer() を呼ぶこと
/// </summary>
/// <param name="CmdList">コピー命令を記録するコマンドリスト</param>
/// <param name="InitData">初期データへのポインタ</param>
/// <param name="Size">バッファのバイトサイズ</param>
/// <param name="Format">DXGI_FORMAT_R16_UINT または DXGI_FORMAT_R32_UINT</param>
/// <returns>true: 成功</returns>
	bool IndexBuffer::CreateStatic(ID3D12GraphicsCommandList* CmdList, const void* InitData, const size_t Size, const DXGI_FORMAT Format)
	{
		mBufferSize = Size;
		mFormat = Format;
		mIsDynamic = false;

		auto device = graphics::DX12Device::Get().GetDevice();
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferSize);

		// Default ヒープにリソースを作成（最初はコピー先として）
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

		// CPU 書き込み用の Upload ヒープに一時リソースを作成
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

		// GPU 側で Upload -> Default へコピー
		CmdList->CopyBufferRegion(mBufferResource.Get(), 0, mUploadResource.Get(), 0, Size);

		// バッファ状態を「コピー先」→「インデックスバッファとして読み取り可能」に遷移
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

	/// <summary>
	/// 同期的な静的インデックスバッファの作成。
	/// DX12Device の専用アップロードキューを使うため cmdList 不要。
	/// DEFAULT ヒープは D3D12MA で確保し断片化を抑制。
	/// スレッドセーフ (内部の UploadBufferData が mutex で保護)。
	/// </summary>
	bool IndexBuffer::CreateStaticSync(const void* InitData, size_t Size, DXGI_FORMAT Format)
	{
		// ガード
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

		// DEFAULTでインデックスバッファを確保
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

		// キャッシュ
		mBufferView.BufferLocation = mBufferResource->GetGPUVirtualAddress();
		mBufferView.SizeInBytes = static_cast<UINT>(mBufferSize);
		mBufferView.Format = mFormat;

		return true;
	}

	/// <summary>
	/// バッファの解放
	/// 動的バッファのみ Unmap を行う（静的バッファは Map していないため不要）
	/// </summary>
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

	/// <summary>
	/// アップロード用の一時バッファを解放
	/// GPU 転送（コマンドリスト実行）完了後に呼ぶこと
	/// </summary>
	void IndexBuffer::ReleaseUploadBuffer()
	{
		mUploadResource.Reset();
	}

	/// <summary>
	/// CPU 側のデータをバッファへ転送する（動的バッファ専用）
	/// </summary>
	/// <param name="SrcData">転送元データへのポインタ</param>
	/// <param name="Size">転送するバイトサイズ</param>
	/// <param name="Offset">バッファ先頭からの書き込みオフセット</param>
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

	/// <summary>
	/// インデックスバッファビューをコマンドリストにセット
	/// </summary>
	/// <param name="CmdList">描画命令を記録するコマンドリスト</param>
	void IndexBuffer::Set(ID3D12GraphicsCommandList* CmdList) const
	{
		if (CmdList == nullptr) return;
		CmdList->IASetIndexBuffer(&mBufferView);
	}

	/// <summary>
	/// 確保済みバッファの全バイトサイズ取得
	/// </summary>
	size_t IndexBuffer::GetSize() const
	{
		return mBufferSize;
	}

	/// <summary>
	/// インデックス数を取得（全バイト数 / インデックス1個のバイトサイズ）
	/// </summary>
	uint32_t IndexBuffer::GetCount() const
	{
		if (mBufferSize == 0) return 0;
		const size_t indexByteSize = (mFormat == DXGI_FORMAT_R16_UINT) ? sizeof(uint16_t) : sizeof(uint32_t);
		return static_cast<uint32_t>(mBufferSize / indexByteSize);
	}

	/// <summary>
	/// インデックスのフォーマット取得
	/// </summary>
	DXGI_FORMAT IndexBuffer::GetFormat() const
	{
		return mFormat;
	}


}

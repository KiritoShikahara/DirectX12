#include "pch.h"
#include "GraphicsDescriptorHeap.h"
#include "GraphicsDescriptorHeapManager.h"

namespace graphics
{
	// ムーブコンストラクタ
	GDescriptorHeap::GDescriptorHeap(GDescriptorHeap&& other) noexcept
		: mHeapInfo(other.mHeapInfo)
		, mManager(other.mManager)
	{
		other.mHeapInfo = {};
		other.mManager = nullptr;
	}

	// ムーブ代入演算子
	GDescriptorHeap& GDescriptorHeap::operator=(GDescriptorHeap&& other) noexcept
	{
		if (this != &other)
		{
			Release();

			mHeapInfo = other.mHeapInfo;
			mManager = other.mManager;

			other.mHeapInfo = {};
			other.mManager = nullptr;
		}
		return *this;
	}

	// デストラクタ
	GDescriptorHeap::~GDescriptorHeap()
	{
		Release();
	}

	// ディスクリプタヒープの確保
	bool GDescriptorHeap::Create(GDescriptorHeapManager& manager, uint32_t size)
	{
		if (mHeapInfo.IsValid())
		{
			Release();
		}

		mHeapInfo = manager.Issuance(size);

		if (!mHeapInfo.IsValid())
		{
			DEBUG_LOG(sys::eLogLevel::Error, "GDescriptorHeap: Failed to allocate {} slot(s).", size);
			return false;
		}

		mManager = &manager;
		return true;
	}

	// リソースの解放
	void GDescriptorHeap::Release()
	{
		if (mHeapInfo.IsValid() && mManager != nullptr)
		{
			mManager->Discard(mHeapInfo);
		}
		mManager = nullptr;
	}

	// CPU ハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE GDescriptorHeap::GetCpuHandle() const
	{
		if (mManager == nullptr) return { 0 };
		return mManager->GetCpuHandle(mHeapInfo);
	}

	// GPU ハンドルの取得
	D3D12_GPU_DESCRIPTOR_HANDLE GDescriptorHeap::GetGpuHandle() const
	{
		if (mManager == nullptr) return { 0 };
		return mManager->GetGpuHandle(mHeapInfo);
	}

	// 有効性チェック
	bool GDescriptorHeap::IsValid() const
	{
		return mHeapInfo.IsValid() && mManager != nullptr;
	}

	// スロットインデックスの取得
	int GDescriptorHeap::GetIndex() const
	{
		return mHeapInfo.Index;
	}
}
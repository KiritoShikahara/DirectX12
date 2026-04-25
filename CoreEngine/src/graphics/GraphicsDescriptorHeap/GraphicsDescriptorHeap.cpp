#include"pch.h"
#include "GraphicsDescriptorHeap.h"
#include"GraphicsDescriptorHeapManager.h"

namespace graphics
{
	GDescriptorHeap::~GDescriptorHeap()
	{
		Release();
	}

	GDescriptorHeap::GDescriptorHeap(GDescriptorHeap&& other) noexcept
		: mHeapInfo(other.mHeapInfo)
		, mManager(other.mManager)
	{
		// 移譲元を無効化して二重解放を防ぐ
		other.mHeapInfo = {};
		other.mManager = nullptr;
	}

	bool GDescriptorHeap::Create(GDescriptorHeapManager& manager, uint32_t size)
	{
		// 既に確保済みなら一度解放してから再確保する
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

		// 確保が成功したときだけ Manager を保持する
		mManager = &manager;
		return true;
	}

	void GDescriptorHeap::Release()
	{
		if (mHeapInfo.IsValid() && mManager != nullptr)
		{
			mManager->Discard(mHeapInfo);
		}
		// Discard() 内で mHeapInfo はリセットされるが、明示的に Manager もクリアする
		mManager = nullptr;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GDescriptorHeap::GetCpuHandle() const
	{
		if (mManager == nullptr) return { 0 };
		return mManager->GetCpuHandle(mHeapInfo);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GDescriptorHeap::GetGpuHandle() const
	{
		if (mManager == nullptr) return { 0 };
		return mManager->GetGpuHandle(mHeapInfo);
	}

	bool GDescriptorHeap::IsValid() const
	{
		return mHeapInfo.IsValid() && mManager != nullptr;
	}
}



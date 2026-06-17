#include"pch.h"
#include "GraphicsDescriptorHeap.h"
#include"GraphicsDescriptorHeapManager.h"

namespace graphics
{
	/// <summary>
	/// ムーブコンストラクタ（所有権の移譲）
	/// </summary>
	GDescriptorHeap::GDescriptorHeap(GDescriptorHeap&& other) noexcept
		: mHeapInfo(other.mHeapInfo)
		, mManager(other.mManager)
	{
		// 移動元のオブジェクトを安全な状態（未確保）にクリアする
		other.mHeapInfo = {};
		other.mManager = nullptr;
	}

	/// <summary>
	/// ムーブ代入演算子（既存リソースの自動解放と所有権移譲）
	/// </summary>
	GDescriptorHeap& GDescriptorHeap::operator=(GDescriptorHeap&& other) noexcept
	{
		if (this != &other)
		{
			// 自分が既にデスクリプタを保持しているなら、安全に解放する
			Release();

			// 移動元から情報をコピー
			mHeapInfo = other.mHeapInfo;
			mManager = other.mManager;

			// 移動元のオブジェクトをクリアして、二重解放を防ぐ
			other.mHeapInfo = {};
			other.mManager = nullptr;
		}
		return *this;
	}

	GDescriptorHeap::~GDescriptorHeap()
	{
		Release();
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

	int GDescriptorHeap::GetIndex() const
	{
		return mHeapInfo.Index;
	}
}



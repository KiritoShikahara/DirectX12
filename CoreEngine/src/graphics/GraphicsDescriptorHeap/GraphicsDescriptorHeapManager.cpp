#include "pch.h"
#include "GraphicsDescriptorHeapManager.h"

namespace graphics
{
	// マネージャーの初期化処理
	bool GDescriptorHeapManager::Initialize(ID3D12Device* device)
	{
		if (device == nullptr)
		{
			return false;
		}

		// ヒープの設定と作成
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = MAX_DESCRIPTOR;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask = 0;

		const HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap));
		if (FAILED(hr))
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Failed to create descriptor heap. HRESULT: 0x%08X", hr);
			return false;
		}

		// ディスクリプタのサイズ取得とハンドルテーブルの事前計算
		mDescriptorSize = device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		const auto cpuStart = mHeap->GetCPUDescriptorHandleForHeapStart();
		const auto gpuStart = mHeap->GetGPUDescriptorHandleForHeapStart();

		for (int i = 0; i < MAX_DESCRIPTOR; ++i)
		{
			const auto offset = static_cast<SIZE_T>(i) * mDescriptorSize;
			mHandles[i].cpu.ptr = cpuStart.ptr + offset;
			mHandles[i].gpu.ptr = gpuStart.ptr + offset;
			mIsUse[i] = false;
		}

		mSearchOffset = 0;
		return true;
	}

	// 終了処理とリーク検知
	void GDescriptorHeapManager::Finalize()
	{
		bool leaked = false;
		for (int i = 0; i < MAX_DESCRIPTOR; ++i)
		{
			if (mIsUse[i])
			{
				DEBUG_LOG(sys::eLogLevel::Error, "Descriptor leak detected at index: {}", i);
				leaked = true;
			}
		}

		if (leaked)
		{
			DEBUG_LOG(sys::eLogLevel::Error, "Some descriptors were not freed!");
		}

		// ヒープのリセット
		mHeap.Reset();
	}

	// 連続したスロットの確保
	GDescriptorHeapInfo graphics::GDescriptorHeapManager::Issuance(uint32_t Size)
	{
		// 要求サイズの妥当性チェック
		if (Size == 0 || static_cast<int>(Size) > MAX_DESCRIPTOR)
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"GDescriptorHeapManager: Invalid Size= %d.", Size);

			return { -1, 0 };
		}

		std::lock_guard<std::mutex> lock(mMutex);

		// Next-Fit アルゴリズムで空き領域を探索
		for (int count = 0; count < MAX_DESCRIPTOR; )
		{
			const int current = (mSearchOffset + count) % MAX_DESCRIPTOR;

			// ヒープの端をまたぐ場合は先頭にラップする
			if (current + static_cast<int>(Size) > MAX_DESCRIPTOR)
			{
				count += (MAX_DESCRIPTOR - current);
				continue;
			}

			// 要求サイズ分の連続した空きがあるか確認
			int conflictAt = -1;
			for (uint32_t s = 0; s < Size; ++s)
			{
				if (mIsUse[current + s])
				{
					conflictAt = static_cast<int>(s);
					break;
				}
			}

			if (conflictAt < 0)
			{
				// 確保できたスロットを使用中にマーク
				for (uint32_t i = 0; i < Size; ++i) mIsUse[current + i] = true;
				mSearchOffset = (current + static_cast<int>(Size)) % MAX_DESCRIPTOR;
				return { current, static_cast<int>(Size) };
			}

			// 競合した位置の次から探索を再開
			count += (conflictAt + 1);
		}

		DEBUG_LOG(sys::eLogLevel::Warning,
			"GDescriptorHeapManager: No free descriptors! requested={}", Size);

		return { -1, 0 };
	}

	// 確保したスロットの返却
	void GDescriptorHeapManager::Discard(GDescriptorHeapInfo& Info)
	{
		if (!Info.IsValid()) return;

		std::lock_guard<std::mutex> lock(mMutex);

		// 範囲内の使用中フラグを下ろす
		const int end = std::min(Info.Index + Info.Size, MAX_DESCRIPTOR);
		for (int i = Info.Index; i < end; ++i)
		{
			mIsUse[i] = false;
		}

		Info = {};
	}

	// CPU ハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE GDescriptorHeapManager::GetCpuHandle(const GDescriptorHeapInfo& info) const
	{
		if (!info.IsValid() || info.Index >= MAX_DESCRIPTOR)
		{
			return { 0 };
		}
		return mHandles[info.Index].cpu;
	}

	// GPU ハンドルの取得
	D3D12_GPU_DESCRIPTOR_HANDLE GDescriptorHeapManager::GetGpuHandle(const GDescriptorHeapInfo& info) const
	{
		if (!info.IsValid() || info.Index >= MAX_DESCRIPTOR)
		{
			return { 0 };
		}
		return mHandles[info.Index].gpu;
	}

	// ネイティブヒープの取得
	ID3D12DescriptorHeap* GDescriptorHeapManager::GetNativeHeap() const
	{
		return mHeap.Get();
	}

}
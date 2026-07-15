#include "pch.h"
#include "GraphicsDescriptorHeapManager.h"


namespace graphics
{
	/// <summary>
	/// 初期化。
	/// ServiceLocator を使わずデバイスを直接受け取る。
	/// </summary>
	/// <param name="device">初期化済みの D3D12 デバイス</param>
	/// <returns>true:成功</returns>
	bool GDescriptorHeapManager::Initialize(ID3D12Device* device)
	{
		if (device == nullptr)
		{
			return false;
		}

		// ヒープの作成
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

		// ハンドルテーブルの事前計算
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

		// 必要であればここで mHeap も明示的に Release する処理を入れる
		mHeap.Reset();
	}

	/// <summary>
	/// 連続した Size スロットを確保して返す。
	/// 失敗時は IsValid() == false の Info を返す。
	/// </summary>
	GDescriptorHeapInfo graphics::GDescriptorHeapManager::Issuance(uint32_t Size)
	{
		// Size == 0 は呼び出し側のバグ
		if (Size == 0 || static_cast<int>(Size) > MAX_DESCRIPTOR)
		{
			DEBUG_LOG(sys::eLogLevel::Error,
				"GDescriptorHeapManager: Invalid Size= %d.", Size);

			return { -1, 0 };
		}

		std::lock_guard<std::mutex> lock(mMutex);

		// Next-Fit でフリーな連続スロットを探す
		for (int count = 0; count < MAX_DESCRIPTOR; )
		{
			const int current = (mSearchOffset + count) % MAX_DESCRIPTOR;

			// ヒープ末尾をまたぐ確保は不可（連続性が保証されないため）
			if (current + static_cast<int>(Size) > MAX_DESCRIPTOR)
			{
				count += (MAX_DESCRIPTOR - current); // 先頭にラップ
				continue;
			}

			// 要求サイズ分の連続空きを確認
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
				// 確保成功
				for (uint32_t i = 0; i < Size; ++i) mIsUse[current + i] = true;
				mSearchOffset = (current + static_cast<int>(Size)) % MAX_DESCRIPTOR;
				return { current, static_cast<int>(Size) };
			}

			// 衝突スロットの次から再探索
			count += (conflictAt + 1);
		}

		DEBUG_LOG(sys::eLogLevel::Warning,
			"GDescriptorHeapManager: No free descriptors! requested={}", Size);

		return { -1, 0 };
	}

	/// <summary>
	/// 確保したスロットを返却する。
	/// 返却後は Info が無効化される。
	/// </summary>
	void GDescriptorHeapManager::Discard(GDescriptorHeapInfo& Info)
	{
		if (!Info.IsValid()) return;

		std::lock_guard<std::mutex> lock(mMutex);

		// 範囲外への書き込みを防ぐ
		const int end = std::min(Info.Index + Info.Size, MAX_DESCRIPTOR);
		for (int i = Info.Index; i < end; ++i)
		{
			mIsUse[i] = false;
		}

		// 返却後は無効化する
		Info = {};
	}

	/// <summary>CPU ハンドルの取得</summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GDescriptorHeapManager::GetCpuHandle(const GDescriptorHeapInfo& info) const
	{
		if (!info.IsValid() || info.Index >= MAX_DESCRIPTOR)
		{
			return { 0 };
		}
		return mHandles[info.Index].cpu;
	}

	/// <summary>GPU ハンドルの取得</summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GDescriptorHeapManager::GetGpuHandle(const GDescriptorHeapInfo& info) const
	{
		if (!info.IsValid() || info.Index >= MAX_DESCRIPTOR)
		{
			return { 0 };
		}
		return mHandles[info.Index].gpu;
	}

	/// <summary>ネイティブのヒープポインタ取得（コマンドリストへのセット用）</summary>
	ID3D12DescriptorHeap* GDescriptorHeapManager::GetNativeHeap() const
	{
		return mHeap.Get();
	}

}


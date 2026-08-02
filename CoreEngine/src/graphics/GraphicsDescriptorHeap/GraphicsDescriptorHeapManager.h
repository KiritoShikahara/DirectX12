#pragma once

#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapInfo.h>
#include <Utility/Export/Export.h>
#include <Utility/Singleton/Singleton.hpp>
#include <graphics/Dx12/Dx12Type.h>
#include <array>
#include <d3d12.h>
#include <mutex>

namespace graphics
{
	class ENGINE_API GDescriptorHeapManager : public utility::Singleton<GDescriptorHeapManager>
	{
		SINGLETON_CLASS(GDescriptorHeapManager);
	public:
		SINGLETON_ACCESSOR(GDescriptorHeapManager);

		/// <summary>
		/// 管理するスロットの最大数
		/// </summary>
		static constexpr int MAX_DESCRIPTOR = 4096;

		/// <summary>
		/// 初期化
		/// </summary>
		bool Initialize(ID3D12Device* device);

		/// <summary>
		/// 終了処理
		/// </summary>
		void Finalize();

		/// <summary>
		/// 連続した Size スロットを確保して返す
		/// </summary>
		[[nodiscard]] GDescriptorHeapInfo Issuance(uint32_t Size);

		/// <summary>
		/// 確保したスロットを返却する
		/// </summary>
		void Discard(GDescriptorHeapInfo& Info);

		/// <summary>
		/// CPU ハンドルの取得
		/// </summary>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const GDescriptorHeapInfo& info) const;

		/// <summary>
		/// GPU ハンドルの取得
		/// </summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const GDescriptorHeapInfo& info) const;

		/// <summary>
		/// ネイティブのヒープポインタ取得
		/// </summary>
		ID3D12DescriptorHeap* GetNativeHeap() const;

	private:
		/// <summary>
		/// 事前計算済みのハンドルペア
		/// </summary>
		struct HandleInfo
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cpu = {};
			D3D12_GPU_DESCRIPTOR_HANDLE gpu = {};
		};

		/// <summary>
		/// CBV/SRV/UAV ヒープ本体
		/// </summary>
		Heap mHeap;

		/// <summary>
		/// 全スロットのハンドルテーブル
		/// </summary>
		std::array<HandleInfo, MAX_DESCRIPTOR>  mHandles = {};

		/// <summary>
		/// 使用中フラグ
		/// </summary>
		std::array<bool, MAX_DESCRIPTOR> mIsUse = {};

		/// <summary>
		/// ディスクリプタ1個分のバイトサイズ
		/// </summary>
		uint32_t mDescriptorSize = 0;

		/// <summary>
		/// 次の空き検索を始めるオフセット
		/// </summary>
		int mSearchOffset = 0;

		/// <summary>
		/// 排他制御用ミューテックス
		/// </summary>
		std::mutex mMutex;
	};
}
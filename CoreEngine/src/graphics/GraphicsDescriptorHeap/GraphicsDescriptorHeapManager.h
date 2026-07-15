#pragma once

#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapInfo.h>
#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>
#include<graphics/Dx12/Dx12Type.h>

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

		/// <summary>管理するスロットの最大数</summary>
		static constexpr int MAX_DESCRIPTOR = 4096;

		/// <summary>
		/// 初期化。
		/// ServiceLocator を使わずデバイスを直接受け取る。
		/// </summary>
		/// <param name="device">初期化済みの D3D12 デバイス</param>
		/// <returns>true:成功</returns>
		bool Initialize(ID3D12Device* device);

		/// <summary>
		/// 終了処理
		/// </summary>
		void Finalize();

		/// <summary>
		/// 連続した Size スロットを確保して返す。
		/// 失敗時は IsValid() == false の Info を返す。
		/// </summary>
		[[nodiscard]] GDescriptorHeapInfo Issuance(uint32_t Size);

		/// <summary>
		/// 確保したスロットを返却する。
		/// 返却後は Info が無効化される。
		/// </summary>
		void Discard(GDescriptorHeapInfo& Info);

		/// <summary>CPU ハンドルの取得</summary>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const GDescriptorHeapInfo& info) const;

		/// <summary>GPU ハンドルの取得</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const GDescriptorHeapInfo& info) const;

		/// <summary>ネイティブのヒープポインタ取得（コマンドリストへのセット用）</summary>
		ID3D12DescriptorHeap* GetNativeHeap() const;

	private:
		/// <summary>事前計算済みのハンドルペア</summary>
		struct HandleInfo
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cpu = {};
			D3D12_GPU_DESCRIPTOR_HANDLE gpu = {};
		};

		/// <summary>CBV/SRV/UAV ヒープ本体</summary>
		Heap mHeap;

		/// <summary>全スロットのハンドルテーブル（初期化時に一括計算）</summary>
		std::array<HandleInfo, MAX_DESCRIPTOR>  mHandles = {};

		/// <summary>使用中フラグ。true = 使用中</summary>
		std::array<bool, MAX_DESCRIPTOR> mIsUse = {};

		/// <summary>ディスクリプタ1個分のバイトサイズ</summary>
		uint32_t mDescriptorSize = 0;

		/// <summary>次の空き検索を始めるオフセット（Next-Fit）</summary>
		int mSearchOffset = 0;

		/// <summary>
		/// mIsUse / mSearchOffset を保護する排他制御。
		/// Issuance/Discard を複数スレッドから同時に呼んだ場合のスロット破損を防ぐ。
		/// </summary>
		std::mutex mMutex;
	};
}
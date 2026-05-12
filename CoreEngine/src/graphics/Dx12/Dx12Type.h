#pragma once

#include<d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include<DirectX12MA/D3D12MemAlloc.h>

namespace graphics
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// デバイス・基盤
	using Device = ComPtr<ID3D12Device8>;
	using Factory = ComPtr<IDXGIFactory7>;
	using Adapter = ComPtr<IDXGIAdapter4>;
	using SwapChain = ComPtr<IDXGISwapChain4>;

	// コマンド
	using CmdList = ComPtr<ID3D12GraphicsCommandList6>;
	using CmdQueue = ComPtr<ID3D12CommandQueue>;
	using CmdAlloc = ComPtr<ID3D12CommandAllocator>;

	// D3D12MA
	using MAAllocator = ComPtr<D3D12MA::Allocator>;
	using MAPool = ComPtr<D3D12MA::Pool>;
	using MAAllocation = ComPtr<D3D12MA::Allocation>;

	// リソース・パイプライン
	using Resource = ComPtr<ID3D12Resource>;
	using Heap = ComPtr<ID3D12DescriptorHeap>;
	using RootSig = ComPtr<ID3D12RootSignature>;
	using PSO = ComPtr<ID3D12PipelineState>;
	using Blob = ComPtr<ID3DBlob>;

	// 同期デバッグ
	using Fence = ComPtr<ID3D12Fence>;
	using Debug5 = ComPtr<ID3D12Debug5>;

	//	デバック
	using DebugDevice = ComPtr<ID3D12DebugDevice2>;
	using InfoQueue = ComPtr<ID3D12InfoQueue1>;

	// フレームバッファの数
	inline constexpr uint32_t FRAME_COUNT = 3;
}
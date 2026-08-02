#include "pch.h"
#include "GpuProfiler.h"

#include <d3dx12.h>

namespace graphics
{
    bool GpuProfiler::Initialize(ID3D12Device* device, ID3D12CommandQueue* queue)
    {
        if (mIsInitialized) return true;
        if (device == nullptr || queue == nullptr) return false;

        // タイムスタンプの単位はGPU固有のカウンタのため、周波数を取得して秒へ換算する。
        if (FAILED(queue->GetTimestampFrequency(&mTimestampFrequency)) || mTimestampFrequency == 0)
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                "GpuProfiler: Timestamp queries are not supported on this queue. GPU timing disabled.");
            return false;
        }

        D3D12_QUERY_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        heapDesc.Count = kQueryCount;
        heapDesc.NodeMask = 0;

        if (FAILED(device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&mQueryHeap))))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "GpuProfiler: Failed to create query heap.");
            return false;
        }

        // 読み戻し用バッファ
        const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint64_t) * kQueryCount);

        if (FAILED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&mReadbackBuffer))))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "GpuProfiler: Failed to create readback buffer.");
            mQueryHeap.Reset();
            return false;
        }

        mFrameRecorded.fill(false);
        mChannelMs.fill(0.0f);

        mIsInitialized = true;
        DEBUG_LOG(sys::eLogLevel::Log, "GpuProfiler: Initialized (timestamp frequency = {} Hz).",
            mTimestampFrequency);
        return true;
    }

    void GpuProfiler::Finalize()
    {
        mReadbackBuffer.Reset();
        mQueryHeap.Reset();
        mIsInitialized = false;
    }

    void GpuProfiler::BeginFrame(uint32_t frameIndex)
    {
        if (!mIsInitialized) return;

        mFrameIndex = frameIndex;

        // このフレームインデックスで前回記録した結果を回収する。
        if (!mFrameRecorded[frameIndex]) return;

        const uint32_t first = frameIndex * kQueriesPerFrame;
        const D3D12_RANGE readRange
        {
            sizeof(uint64_t) * first,
            sizeof(uint64_t) * (first + kQueriesPerFrame)
        };

        void* mapped = nullptr;
        if (FAILED(mReadbackBuffer->Map(0, &readRange, &mapped)) || mapped == nullptr) return;

        const uint64_t* timestamps = static_cast<const uint64_t*>(mapped);
        const double msPerTick = 1000.0 / static_cast<double>(mTimestampFrequency);

        uint64_t frameBegin = 0;
        uint64_t frameEnd = 0;
        bool hasFrameBegin = false;

        for (uint32_t i = 0; i < CHANNEL_COUNT; ++i)
        {
            const uint64_t begin = timestamps[QueryIndex(frameIndex, static_cast<eRenderChannel>(i), false)];
            const uint64_t end = timestamps[QueryIndex(frameIndex, static_cast<eRenderChannel>(i), true)];

            // 何も記録されなかったチャネルは0のままになるため、その場合は0msとして扱う
            mChannelMs[i] = (end > begin)
                ? static_cast<float>(static_cast<double>(end - begin) * msPerTick)
                : 0.0f;

            if (begin != 0 && !hasFrameBegin)
            {
                frameBegin = begin;
                hasFrameBegin = true;
            }
            if (end != 0) frameEnd = end;
        }

        mFrameMs = (hasFrameBegin && frameEnd > frameBegin)
            ? static_cast<float>(static_cast<double>(frameEnd - frameBegin) * msPerTick)
            : 0.0f;

        // 書き込みは行っていないのでWrittenRangeは空にする
        const D3D12_RANGE writtenRange{ 0, 0 };
        mReadbackBuffer->Unmap(0, &writtenRange);
    }

    void GpuProfiler::BeginChannel(ID3D12GraphicsCommandList* cmdList, eRenderChannel channel)
    {
        if (!mIsInitialized || cmdList == nullptr) return;

        cmdList->EndQuery(mQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
            QueryIndex(mFrameIndex, channel, false));
    }

    void GpuProfiler::EndChannel(ID3D12GraphicsCommandList* cmdList, eRenderChannel channel)
    {
        if (!mIsInitialized || cmdList == nullptr) return;

        cmdList->EndQuery(mQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
            QueryIndex(mFrameIndex, channel, true));
    }

    void GpuProfiler::ResolveFrame(ID3D12GraphicsCommandList* cmdList)
    {
        if (!mIsInitialized || cmdList == nullptr) return;

        const uint32_t first = mFrameIndex * kQueriesPerFrame;

        cmdList->ResolveQueryData(
            mQueryHeap.Get(),
            D3D12_QUERY_TYPE_TIMESTAMP,
            first,
            kQueriesPerFrame,
            mReadbackBuffer.Get(),
            sizeof(uint64_t) * first);

        mFrameRecorded[mFrameIndex] = true;
    }

    float GpuProfiler::GetChannelMs(eRenderChannel channel) const
    {
        const uint32_t index = static_cast<uint32_t>(channel);
        return (index < CHANNEL_COUNT) ? mChannelMs[index] : 0.0f;
    }
}

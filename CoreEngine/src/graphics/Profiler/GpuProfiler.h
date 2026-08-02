#pragma once

#include <Utility/Export/Export.h>
#include <Utility/Singleton/Singleton.hpp>
#include <graphics/Dx12/Dx12Type.h>

#include <array>
#include <cstdint>

namespace graphics
{
    /// <summary>
	/// GPUの実行時間をタイムスタンプクエリで計測するクラス
    /// </summary>
    class ENGINE_API GpuProfiler : public utility::Singleton<GpuProfiler>
    {
        SINGLETON_CLASS(GpuProfiler);
    public:
        SINGLETON_ACCESSOR(GpuProfiler);

        /// <summary>クエリヒープと読み戻しバッファを作成する</summary>
        bool Initialize(ID3D12Device* device, ID3D12CommandQueue* queue);

        /// <summary>GPUリソースを解放する</summary>
        void Finalize();

        /// <summary>
        /// フレーム開始時に呼ぶ。前回このフレームインデックスで記録した計測結果を回収する。
        /// </summary>
        void BeginFrame(uint32_t frameIndex);

        /// <summary>チャネルのコマンド記録開始位置にタイムスタンプを打つ</summary>
        void BeginChannel(ID3D12GraphicsCommandList* cmdList, eRenderChannel channel);

        /// <summary>チャネルのコマンド記録終了位置にタイムスタンプを打つ</summary>
        void EndChannel(ID3D12GraphicsCommandList* cmdList, eRenderChannel channel);

        /// <summary>
        /// 今フレーム分のクエリ結果を読み戻しバッファへ書き出すコマンドを積む。
        /// </summary>
        void ResolveFrame(ID3D12GraphicsCommandList* cmdList);

        /// <summary>指定チャネルのGPU所要時間(ミリ秒)</summary>
        float GetChannelMs(eRenderChannel channel) const;

        /// <summary>フレーム全体のGPU所要時間(ミリ秒。Pre開始〜Post終了)</summary>
        float GetFrameMs() const { return mFrameMs; }

        /// <summary>計測が利用可能か(初期化に失敗した環境ではfalse)</summary>
        bool IsAvailable() const { return mIsInitialized; }

    private:
        /// <summary>1チャネルにつき開始・終了の2点を打つ</summary>
        static constexpr uint32_t kQueriesPerChannel = 2;
        static constexpr uint32_t kQueriesPerFrame = CHANNEL_COUNT * kQueriesPerChannel;
        static constexpr uint32_t kQueryCount = kQueriesPerFrame * graphics::FRAME_COUNT;

        /// <summary>指定フレーム・指定チャネルの開始クエリの通し番号</summary>
        static uint32_t QueryIndex(uint32_t frameIndex, eRenderChannel channel, bool isEnd)
        {
            return frameIndex * kQueriesPerFrame
                + static_cast<uint32_t>(channel) * kQueriesPerChannel
                + (isEnd ? 1u : 0u);
        }

        Microsoft::WRL::ComPtr<ID3D12QueryHeap> mQueryHeap;
        Resource                                mReadbackBuffer;

        /// <summary>GPUタイムスタンプの1秒あたりのカウント数(所要時間の算出に使う)</summary>
        uint64_t mTimestampFrequency = 0;

        /// <summary>今フレームのインデックス(BeginFrameで更新)</summary>
        uint32_t mFrameIndex = 0;

        /// <summary>回収済みの計測結果(平滑化前の生値)</summary>
        std::array<float, CHANNEL_COUNT> mChannelMs{};
        float mFrameMs = 0.0f;

        /// <summary>そのフレームインデックスで一度でも計測を記録したか(初回の空読み防止)</summary>
        std::array<bool, graphics::FRAME_COUNT> mFrameRecorded{};

        bool mIsInitialized = false;
    };
}

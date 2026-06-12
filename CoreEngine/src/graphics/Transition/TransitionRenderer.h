#pragma once
#include<Utility/Singleton/Singleton.hpp>

#include<graphics/Dx12/Dx12Type.h>
#include<graphics/Color/Color.h>
#include<Utility/Export/Export.h>

namespace graphics
{
    /// <summary>
    /// フルスクリーントランジション描画クラス
    /// </summary>
    class ENGINE_API TransitionRenderer : public utility::Singleton<TransitionRenderer>
    {
        SINGLETON_CLASS(TransitionRenderer);
    public:
        SINGLETON_ACCESSOR(TransitionRenderer);

        /// <summary>
        /// RootSignature + PSO を生成する。
        /// DX12 初期化後に一度だけ呼ぶ。
        /// </summary>
        bool Initialize();

        /// <summary>
        /// フルスクリーンポリゴンを color で塗りつぶす。
        /// color.a == 0 のとき即リターンする。
        /// </summary>
        /// <param name="cmdList">コマンドリスト</param>
        /// <param name="color">描画色（RGBA）。A が不透明度</param>
        void Draw(ID3D12GraphicsCommandList* cmdList, const graphics::Color& color);

        /// <summary>
        /// GPU リソースを解放する。エンジン終了時に呼ぶ。
        /// </summary>
        void Finalize();

    private:
        bool CreateRootSignature(ID3D12Device* device);
        bool CreatePSO(ID3D12Device* device);

    private:
        // Root32BitConstants スロット番号
        static constexpr UINT SLOT_COLOR = 0; // b0: float4(RGBA)

        RootSig mRootSignature;
        PSO     mPSO;
    };
}



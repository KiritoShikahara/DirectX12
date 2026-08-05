#pragma once

#include <graphics/Dx12/Dx12Type.h>
#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>
#include <graphics/Text/Pipeline/TextPipeline.h>
#include <Utility/Singleton/Singleton.hpp>
#include <Utility/Export/Export.h>
#include"../Atlas/TextAtlas.h"

#include <DirectXMath.h>
#include <entt/entt.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace ecs
{
    struct TextComponent;
}

namespace graphics
{
    class DX12Device;
    class GDescriptorHeapManager;
    class ShaderManager;

    struct alignas(256) TextSceneData
    {
        float ScreenW = 1280.f;
        float ScreenH = 720.f;
        float PxRange = 4.f;
        float Threshold = 0.5f;
        float _pad[60] = {};
    };

    struct TextVertex
    {
        float x, y;
        float u, v;
    };

    class ENGINE_API TextRenderer : public utility::Singleton<TextRenderer>
    {
        SINGLETON_CLASS(TextRenderer);
    public:
        SINGLETON_ACCESSOR(TextRenderer);

        // 呼べばフォントアトラスの読み込みまで済ませてしまうので、
        // 呼び出し側で別途ロードしておく必要はない
        bool Initialize(
            DX12Device& device,
            GDescriptorHeapManager& heapManager,
            ShaderManager& shaderManager);

        void Finalize();

        void Begin();
        void UpdateAndDraw(entt::registry& registry);
        void Submit(const std::wstring& text, float x, float y, float size,
            DirectX::XMFLOAT4 color = { 1.f, 1.f, 1.f, 1.f }, float letterSpacing = 0.f);
        void Flush(ID3D12GraphicsCommandList* cmdList);

        // 実際には描画せず、このsizeで描いたときの幅(px)だけ先に知りたいときに使う
        float MeasureWidth(const std::wstring& text, float size, float letterSpacing = 0.f) const;

        // MeasureWidthの縦バージョン。1行分の縦送り量(px)を返す。
        float MeasureLineHeight(float size) const;

        // テキストの見た目の縦中心を狙った位置(目標Y)にきっちり合わせたいときのオフセット。
        float MeasureVerticalCenterOffset(float size) const;

        // ベースライン(TextComponent::Y)から文字の見た目の下端までの距離。常に正の値。
        float MeasureDescent(float size) const;

        void OnResize(uint32_t w, uint32_t h);

    private:
        struct DrawCall
        {
            uint32_t          VertexStart = 0;
            uint32_t          VertexCount = 0;
            DirectX::XMFLOAT4 Color = {};
        };

        // UpdateAndDraw() 内で収集する描画対象1件分
        struct RenderItem
        {
            const ecs::TextComponent* Label = nullptr;
        };

        bool BuildConstantBuffer(DX12Device& device, GDescriptorHeapManager& heapManager);
        bool BuildVertexBuffer(DX12Device& device);

        static uint32_t GetCurrentFrameIndex();

        std::unique_ptr<TextPipeline> mPipeline;
        std::unique_ptr<TextAtlas>    mAtlas;       // Renderer が所有

        // CB/VB は FRAME_COUNT(トリプルバッファ) 分だけ個別に持つための構造体
        struct FrameBuffer
        {
            Resource        CbResource;
            MAAllocation    CbAllocation;
            GDescriptorHeap CbvHeap;
            TextSceneData* CbMapped = nullptr;

            Resource     VbResource;
            MAAllocation VbAllocation;
            TextVertex* VbMapped = nullptr;
        };
        std::array<FrameBuffer, FRAME_COUNT> mFrames;

        static constexpr uint32_t MAX_CHARS = 4096;
        static constexpr uint32_t VERTS_PER_CHAR = 6;

        std::vector<DrawCall> mDrawCalls;
        uint32_t              mVertexCursor = 0;

        // UpdateAndDraw()の一時バッファ。
        std::vector<RenderItem> mRenderItems;

        GDescriptorHeapManager* mHeapManager = nullptr;
        uint32_t mScreenW = 1280;
        uint32_t mScreenH = 720;
        bool     mIsInitialized = false;
    };

} // namespace graphics
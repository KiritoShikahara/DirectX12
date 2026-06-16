#pragma once

#include <graphics/Dx12/Dx12Type.h>
#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>
#include <graphics/Text/Pipeline/TextPipeline.h>
#include <Utility/Singleton/Singleton.hpp>
#include <Utility/Export/Export.h>
#include"../Atlas/TextAtlas.h"

#include <DirectXMath.h>
#include <entt/entt.hpp>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

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

        /// <summary>
        /// 初期化。アトラスのロードも内部で行う。
        /// </summary>
        bool Initialize(
            DX12Device& device,
            GDescriptorHeapManager& heapManager,
            ShaderManager& shaderManager);

        void Finalize();

        void Begin();
        void UpdateAndDraw(entt::registry& registry);
        void Submit(const std::wstring& text, float x, float y, float size,
            DirectX::XMFLOAT4 color = { 1.f, 1.f, 1.f, 1.f });
        void Flush(ID3D12GraphicsCommandList* cmdList);

        void OnResize(uint32_t w, uint32_t h);

    private:
        struct DrawCall
        {
            uint32_t          VertexStart = 0;
            uint32_t          VertexCount = 0;
            DirectX::XMFLOAT4 Color = {};
        };

        bool BuildConstantBuffer(DX12Device& device, GDescriptorHeapManager& heapManager);
        bool BuildVertexBuffer(DX12Device& device);

        std::unique_ptr<TextPipeline> mPipeline;
        std::unique_ptr<TextAtlas>    mAtlas;       // ← Renderer が所有

        Resource        mCbResource;
        GDescriptorHeap mCbvHeap;
        TextSceneData* mCbMapped = nullptr;

        static constexpr uint32_t MAX_CHARS = 4096;
        static constexpr uint32_t VERTS_PER_CHAR = 6;
        Resource    mVbResource;
        TextVertex* mVbMapped = nullptr;

        std::vector<DrawCall> mDrawCalls;
        uint32_t              mVertexCursor = 0;

        GDescriptorHeapManager* mHeapManager = nullptr;
        uint32_t mScreenW = 1280;
        uint32_t mScreenH = 720;
        bool     mIsInitialized = false;
    };

} // namespace graphics
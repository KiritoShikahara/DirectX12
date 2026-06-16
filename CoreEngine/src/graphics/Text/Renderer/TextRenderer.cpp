#include "pch.h"
#include "TextRenderer.h"
#include"../Atlas/TextAtlas.h"

#include <graphics/Dx12/Dx12Device.h>
#include <graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeapManager.h>
#include <graphics/Shader/ShaderManager.h>
#include <ecs/component/text/TextComponent.h>
#include<system/Window/Window.h>
#include<system/AssetPath/AssetPathManager.h>

#include <d3dx12.h>
#include <algorithm>

namespace graphics
{
    bool TextRenderer::Initialize(
        DX12Device& device,
        GDescriptorHeapManager& heapManager,
        ShaderManager& shaderManager)
    {
        if (mIsInitialized) return false;

        auto& window = sys::Window::Get();

        mHeapManager = &heapManager;
        mScreenW = window.GetVirtualWidth();
        mScreenH = window.GetVirtualHeight();

        auto pngPath = ASSET_PATH("/Engine/Assets/Fonts/font.png");
        auto jsonPath = ASSET_PATH("/Engine/Assets/Fonts/font.json").string();

        // Atlas のロード
        mAtlas = std::make_unique<TextAtlas>();
        if (!mAtlas->Load(pngPath, jsonPath))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "TextRenderer: Failed to load atlas.");
            return false;
        }

        // Pipeline
        mPipeline = std::make_unique<TextPipeline>();
        if (!mPipeline->Create(device, shaderManager))
        {
            DEBUG_LOG(sys::eLogLevel::Error, "TextRenderer: Failed to create pipeline.");
            return false;
        }

        if (!BuildConstantBuffer(device, heapManager)) return false;
        if (!BuildVertexBuffer(device))                return false;

        mDrawCalls.reserve(256);
        mIsInitialized = true;
        DEBUG_LOG(sys::eLogLevel::Log, "TextRenderer: Initialized successfully.");
        return true;
    }

    void TextRenderer::Finalize()
    {
        if (!mIsInitialized) return;

        if (mCbResource && mCbMapped) { mCbResource->Unmap(0, nullptr); mCbMapped = nullptr; }
        if (mVbResource && mVbMapped) { mVbResource->Unmap(0, nullptr); mVbMapped = nullptr; }

        mCbvHeap.Release();
        mCbResource.Reset();
        mVbResource.Reset();
        mPipeline.reset();
        mAtlas.reset();

        mIsInitialized = false;
        DEBUG_LOG(sys::eLogLevel::Log, "TextRenderer: Finalized.");
    }

    void TextRenderer::Begin()
    {
        mDrawCalls.clear();
        mVertexCursor = 0;
    }

    void TextRenderer::UpdateAndDraw(entt::registry& registry)
    {
        if (!mIsInitialized || !mAtlas->IsLoaded()) return;

        struct RenderItem { const ecs::TextComponent* label; };
        std::vector<RenderItem> items;

        registry.view<ecs::TextComponent>().each([&](const ecs::TextComponent& label)
            {
                if (!label.IsVisible || label.Text.empty()) return;
                items.push_back({ &label });
            });

        if (items.empty()) return;

        std::sort(items.begin(), items.end(), [](const RenderItem& a, const RenderItem& b)
            {
                return a.label->Layer < b.label->Layer;
            });

        for (const auto& item : items)
        {
            Submit(item.label->Text,
                item.label->X, item.label->Y,
                item.label->Size,
                item.label->Color);
        }
    }

    void TextRenderer::Submit(
        const std::wstring& text,
        float x, float y,
        float size,
        DirectX::XMFLOAT4 color)
    {
        if (!mIsInitialized || !mAtlas->IsLoaded()) return;

        const float    scale = size;
        const uint32_t startVertex = mVertexCursor;
        float penX = x, penY = y;

        for (wchar_t wc : text)
        {
            const uint32_t cp = static_cast<uint32_t>(wc);

            if (cp == L'\n')
            {
                penX = x;
                penY += size * mAtlas->GetLineHeight();
                continue;
            }

            const GlyphInfo* g = mAtlas->GetGlyph(cp);
            if (!g)
            {
                if (auto* sp = mAtlas->GetGlyph(0x20)) penX += sp->advance * scale;
                continue;
            }

            if (g->planeWidth > 0.f && g->planeHeight > 0.f
                && mVertexCursor + VERTS_PER_CHAR <= MAX_CHARS * VERTS_PER_CHAR)
            {
                const float qL = penX + g->planeBearingX * scale;
                const float qT = penY - g->planeBearingY * scale;
                const float qR = qL + g->planeWidth * scale;
                const float qB = qT + g->planeHeight * scale;

                TextVertex* dst = mVbMapped + mVertexCursor;
                dst[0] = { qL, qT, g->uvX0, g->uvY0 };
                dst[1] = { qR, qT, g->uvX1, g->uvY0 };
                dst[2] = { qR, qB, g->uvX1, g->uvY1 };
                dst[3] = { qL, qT, g->uvX0, g->uvY0 };
                dst[4] = { qR, qB, g->uvX1, g->uvY1 };
                dst[5] = { qL, qB, g->uvX0, g->uvY1 };
                mVertexCursor += VERTS_PER_CHAR;
            }

            penX += g->advance * scale;

        }


        const uint32_t vertCount = mVertexCursor - startVertex;
        if (vertCount == 0) return;

        mDrawCalls.push_back({ startVertex, vertCount, color });
    }

    void TextRenderer::Flush(ID3D12GraphicsCommandList* cmdList)
    {

       if (!mIsInitialized || mDrawCalls.empty()) return;

        if (mCbMapped)
        {
            mCbMapped->ScreenW = static_cast<float>(mScreenW);
            mCbMapped->ScreenH = static_cast<float>(mScreenH);
            mCbMapped->PxRange = mAtlas->GetPxRange();
            mCbMapped->Threshold = 0.2f;
        }

        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());
        cmdList->SetPipelineState(mPipeline->GetPipelineState());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation = mVbResource->GetGPUVirtualAddress();
        vbv.SizeInBytes = static_cast<UINT>(MAX_CHARS * VERTS_PER_CHAR * sizeof(TextVertex));
        vbv.StrideInBytes = sizeof(TextVertex);
        cmdList->IASetVertexBuffers(0, 1, &vbv);

        cmdList->SetGraphicsRootDescriptorTable(TextPipeline::SLOT_SCENE_CBV, mCbvHeap.GetGpuHandle());
        cmdList->SetGraphicsRootDescriptorTable(TextPipeline::SLOT_ATLAS_SRV, mAtlas->GetSrvGpuHandle());

        for (const DrawCall& dc : mDrawCalls)
        {
            cmdList->SetGraphicsRoot32BitConstants(
                TextPipeline::SLOT_COLOR, 4,
                reinterpret_cast<const void*>(&dc.Color), 0);

            cmdList->DrawInstanced(dc.VertexCount, 1, dc.VertexStart, 0);
        }
    }

    void TextRenderer::OnResize(uint32_t w, uint32_t h) { mScreenW = w; mScreenH = h; }

    bool TextRenderer::BuildConstantBuffer(DX12Device& device, GDescriptorHeapManager& heapManager)
    {
        constexpr UINT64 cbSize = sizeof(TextSceneData);
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
        MAAllocation alloc;
        if (FAILED(device.GetMAAllocator()->CreateResource(
            &allocDesc, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, &alloc, IID_PPV_ARGS(&mCbResource)))) return false;

        if (FAILED(mCbResource->Map(0, nullptr, reinterpret_cast<void**>(&mCbMapped)))) return false;

        *mCbMapped = TextSceneData{};
        mCbMapped->ScreenW = static_cast<float>(mScreenW);
        mCbMapped->ScreenH = static_cast<float>(mScreenH);

        if (!mCbvHeap.Create(heapManager, 1)) return false;

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = mCbResource->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = static_cast<UINT>(cbSize);
        device.GetDevice()->CreateConstantBufferView(&cbvDesc, mCbvHeap.GetCpuHandle());
        return true;
    }

    bool TextRenderer::BuildVertexBuffer(DX12Device& device)
    {
        const UINT64 vbSize = MAX_CHARS * VERTS_PER_CHAR * sizeof(TextVertex);
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
        MAAllocation alloc;
        if (FAILED(device.GetMAAllocator()->CreateResource(
            &allocDesc, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, &alloc, IID_PPV_ARGS(&mVbResource)))) return false;

        return SUCCEEDED(mVbResource->Map(0, nullptr, reinterpret_cast<void**>(&mVbMapped)));
    }

} // namespace graphics
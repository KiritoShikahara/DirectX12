#include "pch.h"
#include "TextRenderer.h"
#include"../Atlas/TextAtlas.h"

#include <graphics/Dx12/Dx12Device.h>
#include <graphics/Dx12/RenderContext.h>
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

        for (auto& frame : mFrames)
        {
            if (frame.CbResource && frame.CbMapped) { frame.CbResource->Unmap(0, nullptr); frame.CbMapped = nullptr; }
            if (frame.VbResource && frame.VbMapped) { frame.VbResource->Unmap(0, nullptr); frame.VbMapped = nullptr; }

            frame.CbvHeap.Release();
            frame.CbResource.Reset();
            frame.CbAllocation.Reset();
            frame.VbResource.Reset();
            frame.VbAllocation.Reset();
        }

        mPipeline.reset();
        mAtlas.reset();

        mIsInitialized = false;
        DEBUG_LOG(sys::eLogLevel::Log, "TextRenderer: Finalized.");
    }

    /// <summary>フレームインフライト中の書き込み先取り違えを防ぐため、現在の描画対象フレーム番号を返す</summary>
    uint32_t TextRenderer::GetCurrentFrameIndex()
    {
        return graphics::RenderContext::Get().GetFrameIndex();
    }

    void TextRenderer::Begin()
    {
        mDrawCalls.clear();
        mVertexCursor = 0;
    }

    void TextRenderer::UpdateAndDraw(entt::registry& registry)
    {
        if (!mIsInitialized || !mAtlas->IsLoaded()) return;

        // 毎フレームのvector生成を避けるため、メンバ変数(mRenderItems)を使い回す
        mRenderItems.clear();
        auto view = registry.view<ecs::TextComponent>();
        mRenderItems.reserve(view.size());

        view.each([&](const ecs::TextComponent& label)
            {
                if (!label.IsVisible || label.Text.empty()) return;
                mRenderItems.push_back({ &label });
            });

        if (mRenderItems.empty()) return;

        std::sort(mRenderItems.begin(), mRenderItems.end(), [](const RenderItem& a, const RenderItem& b)
            {
                return a.Label->Layer < b.Label->Layer;
            });

        for (const auto& item : mRenderItems)
        {
            Submit(item.Label->Text,
                item.Label->X, item.Label->Y,
                item.Label->Size,
                item.Label->Color);
        }
    }

    void TextRenderer::Submit(
        const std::wstring& text,
        float x, float y,
        float size,
        DirectX::XMFLOAT4 color)
    {
        if (!mIsInitialized || !mAtlas->IsLoaded()) return;

        TextVertex* vbMapped = mFrames[GetCurrentFrameIndex()].VbMapped;

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

                TextVertex* dst = vbMapped + mVertexCursor;
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

        FrameBuffer& frame = mFrames[GetCurrentFrameIndex()];

        if (frame.CbMapped)
        {
            frame.CbMapped->ScreenW = static_cast<float>(mScreenW);
            frame.CbMapped->ScreenH = static_cast<float>(mScreenH);
            frame.CbMapped->PxRange = mAtlas->GetPxRange();
            frame.CbMapped->Threshold = 0.2f;
        }

        ID3D12DescriptorHeap* heaps[] = { mHeapManager->GetNativeHeap() };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        cmdList->SetGraphicsRootSignature(mPipeline->GetRootSignature());
        cmdList->SetPipelineState(mPipeline->GetPipelineState());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation = frame.VbResource->GetGPUVirtualAddress();
        vbv.SizeInBytes = static_cast<UINT>(MAX_CHARS * VERTS_PER_CHAR * sizeof(TextVertex));
        vbv.StrideInBytes = sizeof(TextVertex);
        cmdList->IASetVertexBuffers(0, 1, &vbv);

        cmdList->SetGraphicsRootDescriptorTable(TextPipeline::SLOT_SCENE_CBV, frame.CbvHeap.GetGpuHandle());
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

        for (auto& frame : mFrames)
        {
            D3D12MA::ALLOCATION_DESC allocDesc = {};
            allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

            auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
            if (FAILED(device.GetMAAllocator()->CreateResource(
                &allocDesc, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, &frame.CbAllocation, IID_PPV_ARGS(&frame.CbResource)))) return false;

            if (FAILED(frame.CbResource->Map(0, nullptr, reinterpret_cast<void**>(&frame.CbMapped)))) return false;

            *frame.CbMapped = TextSceneData{};
            frame.CbMapped->ScreenW = static_cast<float>(mScreenW);
            frame.CbMapped->ScreenH = static_cast<float>(mScreenH);

            if (!frame.CbvHeap.Create(heapManager, 1)) return false;

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = frame.CbResource->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = static_cast<UINT>(cbSize);
            device.GetDevice()->CreateConstantBufferView(&cbvDesc, frame.CbvHeap.GetCpuHandle());
        }
        return true;
    }

    bool TextRenderer::BuildVertexBuffer(DX12Device& device)
    {
        const UINT64 vbSize = MAX_CHARS * VERTS_PER_CHAR * sizeof(TextVertex);

        for (auto& frame : mFrames)
        {
            D3D12MA::ALLOCATION_DESC allocDesc = {};
            allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

            auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
            if (FAILED(device.GetMAAllocator()->CreateResource(
                &allocDesc, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, &frame.VbAllocation, IID_PPV_ARGS(&frame.VbResource)))) return false;

            if (FAILED(frame.VbResource->Map(0, nullptr, reinterpret_cast<void**>(&frame.VbMapped)))) return false;
        }
        return true;
    }

} // namespace graphics
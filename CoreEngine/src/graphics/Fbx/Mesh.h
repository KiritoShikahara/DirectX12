#pragma once

#include"BinaryModel.h"
#include <vector>
#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{
    // サブメッシュ (マテリアル境界 = PolygonCount ベース)
    struct SubMesh
    {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        int      materialIndex = 0;
    };

    class Mesh
    {
    public:
        bool IsSkinned() const { return m_skinned; }

        // GPU へアップロード (コマンドリスト実行 → フェンス待ち後に EndUpload() を呼ぶ)
        void Upload(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
        void EndUpload();   // アップロードバッファを解放する

        // VB/IB をバインドして全サブメッシュを描画
        void Draw(ID3D12GraphicsCommandList* cmdList) const;

        // BinaryLoader が設定する CPU 側データ
        std::vector<BinVertex> vertices;
        std::vector<uint32_t>  indices;
        std::vector<SubMesh>   subMeshes;
        bool m_skinned = false;

    private:
        Resource m_vb;
        Resource m_ib;
        Resource m_vbUpload;  // 転送中保持
        Resource m_ibUpload;

        D3D12_VERTEX_BUFFER_VIEW m_vbView{};
        D3D12_INDEX_BUFFER_VIEW  m_ibView{};
        bool m_uploaded = false;

        static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
            ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
            const void* data, UINT64 byteSize,
            Resource& uploadBuf);
    };
}
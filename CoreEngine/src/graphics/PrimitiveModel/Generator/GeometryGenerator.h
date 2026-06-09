#pragma once

#include<vector>
#include<graphics/Fbx/Data/FbxData.h>

namespace graphics
{
	class GeometryGenerator
	{
	public:
		GeometryGenerator() = delete;
		virtual ~GeometryGenerator() = delete;

        /// <summary>直方体 (中心原点)</summary>
        static void CreateBox(
            float width, float height, float depth,
            std::vector<FbxVertex>& outVertices,
            std::vector<uint32_t>& outIndices);

        /// <summary>UV球 (中心原点)</summary>
        /// @param slices 経線分割数 (>=3)
        /// @param stacks 緯線分割数 (>=2)
        static void CreateSphere(
            float radius,
            uint32_t slices, uint32_t stacks,
            std::vector<FbxVertex>& outVertices,
            std::vector<uint32_t>& outIndices);

        /// <summary>XZ 平面 (Y=0, 中心原点)</summary>
        static void CreatePlane(
            float width, float depth,
            uint32_t divsX, uint32_t divsZ,
            std::vector<FbxVertex>& outVertices,
            std::vector<uint32_t>& outIndices);

        /// <summary>円柱 (Y軸方向, 中心原点)</summary>
        static void CreateCylinder(
            float topRadius, float bottomRadius,
            float height,
            uint32_t slices, uint32_t stacks,
            std::vector<FbxVertex>& outVertices,
            std::vector<uint32_t>& outIndices);

        /// <summary>カプセル (Y軸方向, 中心原点)</summary>
        /// シリンダー部分の高さ = height、上下に半球を追加
        static void CreateCapsule(
            float radius, float height,
            uint32_t slices, uint32_t hemisphereStacks,
            std::vector<FbxVertex>& outVertices,
            std::vector<uint32_t>& outIndices);

    private:
        // 接線ベクトルを法線から自動計算するユーティリティ
        static DirectX::XMFLOAT3 CalcTangent(const DirectX::XMFLOAT3& normal);

        // ボーンを全部 0 で初期化した頂点を作るユーティリティ
        static FbxVertex MakeVertex(
            const DirectX::XMFLOAT3& pos,
            const DirectX::XMFLOAT2& uv,
            const DirectX::XMFLOAT3& normal,
            const DirectX::XMFLOAT3& tangent);
	};
}



#include "pch.h"
#include "PrimitiveResourceManager.h"

#include"../Generator/GeometryGenerator.h"

namespace graphics
{

    /// <summary>
    /// デフォルトパラメーターでPrimitiveResourceを生成してキャッシュする。
    /// </summary>
    /// <returns></returns>
    void PrimitiveResourceManager::Initialize()
	{
        struct Entry {
            PrimitiveType type;
            const char* name;
            float p0, p1, p2;
            uint32_t div0, div1;
        };

        const Entry entries[] =
        {
            { PrimitiveType::Box,      "Box",      1.f,  1.f,  1.f,  1,   1  },
            { PrimitiveType::Sphere,   "Sphere",   0.5f, 0.f,  0.f,  32,  16 },
            { PrimitiveType::Plane,    "Plane",    1.f,  1.f,  0.f,  1,   1  },
            { PrimitiveType::Plane,    "Field",    100.f, 100.f, 0.f,  20,  20 },
            { PrimitiveType::Cylinder, "Cylinder", 0.5f, 0.5f, 1.f,  32,  1  },
            { PrimitiveType::Capsule,  "Capsule",  0.5f, 1.f,  0.f,  32,  8  },
        };

        for (const auto& e : entries)
        {
            auto res = CreatePrimitive(e.type, e.p0, e.p1, e.p2, e.div0, e.div1);
            if (res)
            {
                mCache[e.name] = std::move(res);
                DEBUG_LOG(sys::eLogLevel::Log,
                    std::format("PrimitiveResourceManager: Created '{}'", e.name));
            }
            else
            {
                DEBUG_LOG(sys::eLogLevel::Error,
                    std::format("PrimitiveResourceManager: Failed to create '{}'", e.name));
            }
        }
	}

    void PrimitiveResourceManager::Finalize()
    {
        mCache.clear();
        DEBUG_LOG(sys::eLogLevel::Log, "PrimitiveResourceManager: Shutdown.");
    }

    /// <summary>
    /// 名前からキャッシュされた PrimitiveResourceを返す。
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    FbxResource* PrimitiveResourceManager::GetResource(const std::string& name) const
    {
        auto it = mCache.find(name);
        if (it == mCache.end())
        {
            DEBUG_LOG(sys::eLogLevel::Warning,
                std::format("PrimitiveResourceManager: '{}' not found.", name));
            return nullptr;
        }
        return it->second.get();
    }

    /// <summary>
    /// カスタムパラメータでプリミティブを生成してキャッシュに登録する
    /// すでに同名のエントリがあれば上書きする
    /// </summary>
    FbxResource* PrimitiveResourceManager::CreateAndRegister(
        PrimitiveType type,
        const std::string& name,
        float p0, float p1, float p2,
        uint32_t div0, uint32_t div1)
    {
        auto res = CreatePrimitive(type, p0, p1, p2, div0, div1);
        if (!res)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                std::format("PrimitiveResourceManager: CreateAndRegister failed for '{}'", name));
            return nullptr;
        }
        FbxResource* ptr = res.get();
        mCache[name] = std::move(res);
        return ptr;
    }

    std::unique_ptr<FbxResource> PrimitiveResourceManager::CreatePrimitive(
        PrimitiveType type,
        float p0, float p1, float p2,
        uint32_t div0, uint32_t div1) const
    {
        std::vector<FbxVertex>  vertices;
        std::vector<uint32_t>   indices;

        switch (type)
        {
        case PrimitiveType::Box:
            GeometryGenerator::CreateBox(p0, p1, p2, vertices, indices);
            break;

        case PrimitiveType::Sphere:
            GeometryGenerator::CreateSphere(p0, div0, div1, vertices, indices);
            break;

        case PrimitiveType::Plane:
            GeometryGenerator::CreatePlane(p0, p1, div0, div1, vertices, indices);
            break;

        case PrimitiveType::Cylinder:
            GeometryGenerator::CreateCylinder(p0, p1, p2, div0, div1, vertices, indices);
            break;

        case PrimitiveType::Capsule:
            GeometryGenerator::CreateCapsule(p0, p1, div0, div1, vertices, indices);
            break;

        default:
            return nullptr;
        }

        if (vertices.empty() || indices.empty()) return nullptr;

        // マテリアルなしのデフォルトセクション
        FbxSection sec = {};
        sec.Name = "Default";
        sec.IndexOffset = 0;
        sec.IndexCount = static_cast<uint32_t>(indices.size());

        auto res = std::make_unique<FbxResource>();
        if (!res->BuildFromMemory(vertices, indices, { sec }))
        {
            return nullptr;
        }

        return res;
    }

}
#include "pch.h"
#include "FbxResourceManager.h"
#include"FbxResource.h"

namespace graphics
{
    std::shared_ptr<FbxResource> FbxResourceManager::Load(
        ID3D12GraphicsCommandList* cmdList,   // ’Ç‰Á
        const std::string& binPath,
        const std::string& anmPath)
    {
        auto it = mCache.find(binPath);
        if (it != mCache.end()) return it->second;

        auto resource = std::make_shared<FbxResource>();
        if (!resource->Load(cmdList, binPath, anmPath))  // cmdList ‚ð“n‚·
        {
            return nullptr;
        }

        mCache.emplace(binPath, resource);
        return resource;
    }

    std::shared_ptr<FbxResource> FbxResourceManager::Get(const std::string& binPath) const
    {
        auto it = mCache.find(binPath);
        return (it != mCache.end()) ? it->second : nullptr;
    }

    void FbxResourceManager::Unload(const std::string& binPath)
    {
        auto it = mCache.find(binPath);
        if (it == mCache.end()) return;

        // ‘¼‚ÉŽQÆ‚ª‚È‚¯‚ê‚Î‰ð•ú
        if (it->second.use_count() <= 1)
        {
            mCache.erase(it);
            DEBUG_LOG(sys::eLogLevel::Log,
                std::format("FbxResourceManager: Unloaded '{}'", binPath));
        }
    }

    void FbxResourceManager::Clear()
    {
        mCache.clear();
    }
}

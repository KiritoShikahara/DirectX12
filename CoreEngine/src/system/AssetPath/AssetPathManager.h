#pragma once
#include <Utility/Export/Export.h>
#include <Utility/Singleton/Singleton.hpp>
#include <filesystem>
#include <unordered_map>
#include <string_view>

namespace sys
{
    class ENGINE_API AssetPathManager : public utility::Singleton<AssetPathManager>
    {
        SINGLETON_CLASS(AssetPathManager);
    public:
        SINGLETON_ACCESSOR(AssetPathManager);

        void Initialize(
            const std::filesystem::path& gameContentDir = {},
            const std::filesystem::path& engineRootDir = {});

        std::filesystem::path Resolve(std::string_view virtualPath) const;
        std::wstring          ResolveW(std::string_view virtualPath) const;

        void DumpRoots() const;

    private:
        bool Split(std::string_view virtualPath,
            std::string& outRoot,
            std::string& outSub) const;

        std::unordered_map<std::string, std::filesystem::path> mRoots;
    };
}

#define ASSET_PATH(vpath)  sys::AssetPathManager::Get().Resolve(vpath)
#define ASSET_PATHW(vpath) sys::AssetPathManager::Get().ResolveW(vpath)
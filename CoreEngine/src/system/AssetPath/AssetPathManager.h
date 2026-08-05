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

        ///<summary>
        ///UTF-8のnarrow文字列で解決結果を返す。std::filesystem::path::string()は実行環境のネイティブコードページ(日本語Windowsでは932)で
        ///変換するため、インストール先のパスに非ASCII文字が含まれるとDXC(UTF-8前提)側で文字化けし読み込みに失敗する。
        ///UTF-8を前提とする呼び出し先(ShaderManager::GetShader等)は必ずこちらを使うこと
        ///</summary>
        std::string ResolveUtf8(std::string_view virtualPath) const;

        void DumpRoots() const;

    private:
        bool Split(std::string_view virtualPath,
            std::string& outRoot,
            std::string& outSub) const;

        std::unordered_map<std::string, std::filesystem::path> mRoots;
    };
}

#define ASSET_PATH(vpath)     sys::AssetPathManager::Get().Resolve(vpath)
#define ASSET_PATHW(vpath)    sys::AssetPathManager::Get().ResolveW(vpath)
#define ASSET_PATH_UTF8(vpath) sys::AssetPathManager::Get().ResolveUtf8(vpath)
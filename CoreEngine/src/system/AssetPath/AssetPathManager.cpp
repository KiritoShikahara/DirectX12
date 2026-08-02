#include "pch.h"
#include "AssetPathManager.h"
#include <iostream>

namespace sys
{
    static std::filesystem::path GetExeDir()
    {
#ifdef _WIN32
        wchar_t buf[MAX_PATH]{};
        GetModuleFileNameW(nullptr, buf, MAX_PATH);
        return std::filesystem::path(buf).parent_path();
#else
        return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
    }

    // 自身、またはその直下のディレクトリにマーカーがあるか探しながら遡る。
    static std::filesystem::path FindRootByMarker(
        const std::filesystem::path& startDir,
        const std::string& marker)
    {
        auto dir = std::filesystem::weakly_canonical(startDir);

        // 階層が深い場合を考慮して20回遡る
        for (int i = 0; i < 20; ++i)
        {
            // このディレクトリ直下にマーカーがあるか
            if (std::filesystem::exists(dir / marker)) {
                return dir;
            }

            // このディレクトリの直下のサブディレクトリにマーカーがあるか
            // これにより、Appと並列にあるEngineフォルダの中のマーカーを見つけられる
            try {
                if (std::filesystem::is_directory(dir)) {
                    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                        if (entry.is_directory() && std::filesystem::exists(entry.path() / marker)) {
                            return entry.path();
                        }
                    }
                }
            }
            catch (...) {}

            auto parent = dir.parent_path();
            if (parent == dir) break;
            dir = parent;
        }
        return {};
    }

    void AssetPathManager::Initialize(const std::filesystem::path& gameContentDir, const std::filesystem::path& engineRootDir)
    {
        auto exeDir = GetExeDir();
        auto cwd = std::filesystem::current_path();

        // Game
        if (!gameContentDir.empty()) {
            mRoots["Game"] = gameContentDir;
        }
        else {
            // cwd(作業ディレクトリ)から探すのが一番確実
            mRoots["Game"] = FindRootByMarker(cwd, ".game_root");
            if (mRoots["Game"].empty()) mRoots["Game"] = FindRootByMarker(exeDir, ".game_root");
        }

        // Engine
        if (!engineRootDir.empty()) {
            mRoots["Engine"] = engineRootDir;
        }
        else {
            // EngineはAppと並列にあるため、共通の親まで遡ってから見つける
            mRoots["Engine"] = FindRootByMarker(exeDir, ".engine_root");
            if (mRoots["Engine"].empty()) mRoots["Engine"] = FindRootByMarker(cwd, ".engine_root");
        }

#ifdef _DEBUG
        DumpRoots();
#endif
    }

    std::filesystem::path AssetPathManager::Resolve(std::string_view virtualPath) const
    {
        std::string root, sub;
        if (!Split(virtualPath, root, sub)) return std::filesystem::path(virtualPath);

        auto it = mRoots.find(root);
        if (it == mRoots.end() || it->second.empty()) return {};

        return sub.empty() ? it->second : (it->second / sub);
    }

    std::wstring AssetPathManager::ResolveW(std::string_view virtualPath) const
    {
        return Resolve(virtualPath).wstring();
    }

    void AssetPathManager::DumpRoots() const
    {
        std::cout << "[AssetPathManager] Mappings:\n";
        for (auto& [name, path] : mRoots)
            std::cout << "  /" << name << "/ -> " << (path.empty() ? "NOT FOUND" : path.string()) << "\n";
    }

    bool AssetPathManager::Split(std::string_view virtualPath, std::string& outRoot, std::string& outSub) const
    {
        if (virtualPath.empty() || virtualPath[0] != '/') return false;
        auto rest = virtualPath.substr(1);
        auto sep = rest.find('/');
        if (sep == std::string_view::npos) {
            outRoot = std::string(rest);
            outSub.clear();
        }
        else {
            outRoot = std::string(rest.substr(0, sep));
            outSub = std::string(rest.substr(sep + 1));
        }
        return !outRoot.empty();
    }
}
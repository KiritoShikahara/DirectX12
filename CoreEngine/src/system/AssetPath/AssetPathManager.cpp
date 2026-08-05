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

    // ���g�A�܂��͂��̒����̃f�B���N�g���Ƀ}�[�J�[�����邩�T���Ȃ���k��B
    static std::filesystem::path FindRootByMarker(
        const std::filesystem::path& startDir,
        const std::string& marker)
    {
        auto dir = std::filesystem::weakly_canonical(startDir);

        // �K�w���[���ꍇ���l������20��k��
        for (int i = 0; i < 20; ++i)
        {
            // ���̃f�B���N�g�������Ƀ}�[�J�[�����邩
            if (std::filesystem::exists(dir / marker)) {
                return dir;
            }

            // ���̃f�B���N�g���̒����̃T�u�f�B���N�g���Ƀ}�[�J�[�����邩
            // ����ɂ��AApp�ƕ���ɂ���Engine�t�H���_�̒��̃}�[�J�[����������
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
            // cwd(��ƃf�B���N�g��)����T���̂���Ԋm��
            mRoots["Game"] = FindRootByMarker(cwd, ".game_root");
            if (mRoots["Game"].empty()) mRoots["Game"] = FindRootByMarker(exeDir, ".game_root");
        }

        // Engine
        if (!engineRootDir.empty()) {
            mRoots["Engine"] = engineRootDir;
        }
        else {
            // Engine��App�ƕ���ɂ��邽�߁A���ʂ̐e�܂ők���Ă��猩����
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

    std::string AssetPathManager::ResolveUtf8(std::string_view virtualPath) const
    {
        const std::wstring wide = ResolveW(virtualPath);
        if (wide.empty()) return {};

        const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (sizeNeeded <= 0) return {};

        std::string utf8(sizeNeeded, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8.data(), sizeNeeded, nullptr, nullptr);

        // WideCharToMultiByteは終端の\0を含むサイズを返すため、末尾を落とす
        if (!utf8.empty() && utf8.back() == '\0')
        {
            utf8.pop_back();
        }
        return utf8;
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
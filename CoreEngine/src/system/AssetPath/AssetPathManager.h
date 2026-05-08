#pragma once
#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>

#include<filesystem>
#include<unordered_map>
#include<string_view>

namespace sys
{
	class ENGINE_API AssetPathManager : public utility::Singleton<AssetPathManager>
	{
		SINGLETON_CLASS(AssetPathManager);
	public:
		SINGLETON_ACCESSOR(AssetPathManager);

		/// <summary>
		/// 初期化
		/// エンジンとゲームのディレクトリをしているする。
		/// </summary>
		/// <param name="gameContentDir"></param>
		/// <param name="engineRootDir"></param>
		void Initialize(
			const std::filesystem::path& gameContentDir = {},
			const std::filesystem::path& engineRootDir = {});

		/// <summary>
		/// パスの追加
		/// 解決できない場合は、空パスを返す
		/// </summary>
		std::filesystem::path Resolve(std::string_view virtualPath) const;
		std::wstring          ResolveW(std::string_view virtualPath) const;

		// デバッグ用: 登録済みルート一覧を標準出力へ
		void DumpRoots() const;
	private:
		// "/Engine/Shader/VS.hlsl" → root="Engine", sub="Shader/VS.hlsl"
		bool Split(std::string_view virtualPath,
			std::string& outRoot,
			std::string& outSub) const;

		// "Engine" → "D:/.../Engine/SystemAssets"
		// "Game"   → "D:/.../Game/Content"
		std::unordered_map<std::string, std::filesystem::path> mRoots;
	};
}

#define ASSET_PATH(vpath) sys::AssetPathManager::Get().Resolve(vpath)
#define ASSET_PATHW(vpath) sys::AssetPathManager::Get().ResolveW(vpath)


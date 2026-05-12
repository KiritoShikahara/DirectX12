#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>

#include<filesystem>
#include<unordered_map>
#include<mutex>

namespace graphics
{

	class Texture;

	class ENGINE_API TextureManager : public utility::Singleton<TextureManager>
	{
		SINGLETON_CLASS(TextureManager);
	public:
		SINGLETON_ACCESSOR(TextureManager);

		/// <summary>
		/// テクスチャの取得、ミロードならロードする。
		/// </summary>
		/// <param name="FilePath">ファイルパス</param>
		/// <returns>参照用のポインタ</returns>
		Texture* GetOrLoad(const std::filesystem::path& FilePath);

		/// <summary>
		/// すべてのテクスチャを解放する。
		/// </summary>
		void Clear();
	private:
		/// <summary>
		/// リソース全体
		/// </summary>
		std::unordered_map<std::string, std::unique_ptr<Texture>> mResources;

		/// <summary>
		/// 排他制御
		/// </summary>
		std::mutex mMutex;

	};

}


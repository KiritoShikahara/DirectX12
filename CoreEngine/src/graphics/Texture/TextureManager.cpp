#include"pch.h"
#include "TextureManager.h"
#include"Texture.h"

namespace graphics
{
	/// <summary>
	/// テクスチャの取得、ミロードならロードする。
	/// </summary>
	/// <param name="FilePath">ファイルパス</param>
	/// <returns>参照用のポインタ</returns>
	Texture* TextureManager::GetOrLoad(const std::filesystem::path& FilePath)
	{
		std::string key = std::filesystem::absolute(FilePath).generic_string();

		// 検索
		{
			std::lock_guard<std::mutex> lock(mMutex);
			auto it = mResources.find(key);
			if (it != mResources.end())
			{
				return it->second.get();
			}
		}

		// ロード
		auto newTexture = std::make_unique<Texture>();
		if (!newTexture->Create(FilePath))
		{
			return nullptr;
		}

		// 登録
		{
			std::lock_guard<std::mutex> lock(mMutex);
			auto [it, inserted] = mResources.emplace(key, std::move(newTexture));
			if (inserted)
			{
				return it->second.get();
			}
		}

		return nullptr;
	}

	/// <summary>
	/// すべてのテクスチャを解放する。
	/// </summary>
	void TextureManager::Clear()
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mResources.clear();
	}
}
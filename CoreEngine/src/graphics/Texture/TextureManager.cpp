#include"pch.h"
#include "TextureManager.h"
#include"Texture.h"

namespace graphics
{
	/// <summary>
	/// �e�N�X�`���̎擾�A�~���[�h�Ȃ烍�[�h����B
	/// </summary>
	/// <param name="FilePath">�t�@�C���p�X</param>
	/// <returns>�Q�Ɨp�̃|�C���^</returns>
	Texture* TextureManager::GetOrLoad(const std::filesystem::path& FilePath, bool isSRGB)
	{
		std::string key = std::filesystem::absolute(FilePath).generic_string();
		// 同じパスでも色空間の解釈(isSRGB)が異なれば別テクスチャとして扱う。
		// (色テクスチャ用とデータテクスチャ用で誤って同一キャッシュを共有しないため)
		if (isSRGB)
		{
			key += "|srgb";
		}

		// ����
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
		if (!newTexture->Create(FilePath, isSRGB))
		{
			return nullptr;
		}

		// 登録
		// 他スレッドが同じキーを先に登録済み(inserted==false)でも、
		// 既存エントリを返す(nullptrを返すと呼び出し側がロード済みテクスチャを取得できなくなる)。
		{
			std::lock_guard<std::mutex> lock(mMutex);
			auto it = mResources.emplace(key, std::move(newTexture)).first;
			return it->second.get();
		}
	}

	/// <summary>
	/// ���ׂẴe�N�X�`�����������B
	/// </summary>
	void TextureManager::Clear()
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mResources.clear();
	}
}
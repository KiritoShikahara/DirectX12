#include"pch.h"
#include "TextureManager.h"
#include"Texture.h"

#include<functional>

namespace graphics
{
	/// <summary>
	/// �e�N�X�`���̎擾�A�~���[�h�Ȃ烍�[�h����B
	/// </summary>
	/// <param name="FilePath">�t�@�C���p�X</param>
	/// <returns>�Q�Ɨp�̃|�C���^</returns>
	Texture* TextureManager::GetOrLoad(const std::filesystem::path& FilePath, bool isSRGB)
	{
		const std::string key = MakeCacheKey(FilePath, isSRGB);

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

	// 同じパスでも色空間の解釈(isSRGB)が異なれば別テクスチャとして扱う。
	// (色テクスチャ用とデータテクスチャ用で誤って同一キャッシュを共有しないため)
	std::string TextureManager::MakeCacheKey(const std::filesystem::path& FilePath, bool isSRGB)
	{
		std::string key = std::filesystem::absolute(FilePath).generic_string();
		if (isSRGB)
		{
			key += "|srgb";
		}
		return key;
	}

	/// <summary>
	/// 複数枚のテクスチャをまとめて先読みする。CPU側の処理(ファイル読み込み・デコード・
	/// ミップ生成)だけをワーカースレッドへ分配して並列実行し、GPUリソース作成(アップロード・
	/// SRV作成)は呼び出し元スレッドで直列に行う(GetOrLoadと同じスレッド安全性の考え方)。
	/// 既にキャッシュ済みのパス(isSRGBの組も一致するもの)はスキップする。
	/// </summary>
	void TextureManager::PreloadBatch(const std::vector<std::filesystem::path>& FilePaths, bool isSRGB)
	{
		if (FilePaths.empty()) return;

		if (!mLoadThreadPoolStarted)
		{
			mLoadThreadPool.Initialize();
			mLoadThreadPoolStarted = true;
		}

		// 未キャッシュのパスだけを対象にする(既存のキャッシュ済みテクスチャを二重ロードしない)
		struct PendingLoad
		{
			std::filesystem::path Path;
			std::string Key;
			Texture::ImageData Data;
		};

		std::vector<PendingLoad> pending;
		pending.reserve(FilePaths.size());
		{
			std::lock_guard<std::mutex> lock(mMutex);
			for (const auto& path : FilePaths)
			{
				std::string key = MakeCacheKey(path, isSRGB);
				if (mResources.find(key) != mResources.end()) continue; // 既にロード済み
				pending.push_back({ path, std::move(key), {} });
			}
		}
		if (pending.empty()) return;

		// CPU側のデコードだけをワーカースレッドへ分配する(D3D12を一切呼ばないため安全)。
		// ThreadPool::Dispatchは1回あたり最大WorkerCount()個までしか受け付けない
		// フォークジョイン専用の作りのため、その単位でチャンク分割して繰り返す。
		const size_t workerCount = std::max<size_t>(mLoadThreadPool.WorkerCount(), 1);
		std::vector<std::function<void()>> tasks(workerCount);

		for (size_t offset = 0; offset < pending.size(); offset += workerCount)
		{
			const size_t chunk = std::min(workerCount, pending.size() - offset);
			for (size_t i = 0; i < chunk; ++i)
			{
				PendingLoad* load = &pending[offset + i];
				tasks[i] = [load, isSRGB]()
					{
						load->Data = Texture::LoadImageData(load->Path, isSRGB);
					};
			}
			mLoadThreadPool.Dispatch(tasks.data(), chunk);
			mLoadThreadPool.WaitAll();
		}

		// GPUリソース作成(アップロード含む)は呼び出し元スレッドで直列に行う
		for (auto& load : pending)
		{
			if (!load.Data.Success) continue;

			auto texture = std::make_unique<Texture>();
			if (!texture->CreateFromImageData(load.Path, isSRGB, load.Data)) continue;

			std::lock_guard<std::mutex> lock(mMutex);
			mResources.emplace(load.Key, std::move(texture));
		}
	}
}
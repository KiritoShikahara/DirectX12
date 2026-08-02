#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>
#include<Utility/Thread/ThreadPool.h>

#include<filesystem>
#include<unordered_map>
#include<vector>
#include<mutex>
#include<functional>

#include<graphics/Texture/Texture.h>

namespace graphics
{

	class ENGINE_API TextureManager : public utility::Singleton<TextureManager>
	{
		SINGLETON_CLASS(TextureManager);
	public:
		SINGLETON_ACCESSOR(TextureManager);

		/// <summary>
		/// テクスチャをキャッシュから取得する。キャッシュに存在しなければロードしてキャッシュに登録する。
		/// </summary>
		/// <param name="FilePath">ファイルパス</param>
		/// <param name="isSRGB">SRGB有効かどうか</param>
		/// <returns></returns>
		Texture* GetOrLoad(const std::filesystem::path& FilePath, bool isSRGB = false);

		/// <summary>
		/// CPU側でテクスチャのリソース一括読み込み。
		/// CPU側とGPU側で分離してCPU側を並列で行えるように。
		/// </summary>
		/// <param name="FilePaths"></param>
		/// <param name="isSRGB"></param>
		void PreloadBatchDecode(const std::vector<std::filesystem::path>& FilePaths, bool isSRGB = false);

		/// <summary>
		/// GPU側のリソース作成・アップロードを行いキャッシュに登録
		/// </summary>
		/// <param name="onItemLoaded"></param>
		void PreloadBatchResolve(const std::function<void()>& onItemLoaded = nullptr);

		/// <summary>
		/// リソース作成
		/// </summary>
		void Clear();
	private:
		/// <summary>
		/// キャッシュのキーの作成
		/// </summary>
		static std::string MakeCacheKey(const std::filesystem::path& FilePath, bool isSRGB);

		struct PendingItem
		{
			std::filesystem::path Path;
			std::string Key;
			bool IsSRGB = false;
			Texture::ImageData Data;
		};

		/// <summary>
		/// リソースのキャッシュ
		/// </summary>
		std::unordered_map<std::string, std::unique_ptr<Texture>> mResources;

		/// <summary>
		/// 非同期処理用
		/// </summary>
		std::mutex mMutex;
		
		/// <summary>
		/// ワーカープール
		/// </summary>
		utility::ThreadPool mLoadThreadPool;
		bool mLoadThreadPoolStarted = false;

		// PreloadBatchDecode()が貯めた、まだGPUリソース化していないデコード結果。
		// PreloadBatchResolve()がメインスレッドで消費してクリアする。
		std::vector<PendingItem> mPendingParsed;
	};

}


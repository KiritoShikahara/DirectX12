#pragma once

#include <Utility/Singleton/Singleton.hpp>
#include <Utility/Thread/ThreadPool.h>
#include <graphics/Fbx/Resource/FbxResource.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <Utility/Export/Export.h>

namespace graphics
{
	class ENGINE_API FbxResourceManager : public utility::Singleton<FbxResourceManager>
	{
		SINGLETON_CLASS(FbxResourceManager);
	public:
		SINGLETON_ACCESSOR(FbxResourceManager);

		/// <summary>
		/// FBX モデルを読み込む
		/// </summary>
		FbxResource* Load(
			const std::string& binPath);

		/// <summary>
		/// 多数の .bin モデルを事前ロードする（2フェーズに分離）
		/// </summary>
		void PreloadBatchParse(const std::vector<std::string>& binPaths,
			const std::function<void(int)>& onTotalDiscovered = nullptr);

		/// <summary>
		/// PreloadBatchParse() で貯めた解析結果をすべて解決しクリアする（メインスレッド専用）
		/// </summary>
		void PreloadBatchResolve(const std::function<void()>& onItemLoaded = nullptr);

		/// <summary>
		/// アニメーションデータを読み込む
		/// </summary>
		bool LoadAnm(
			const std::string& binPath,
			const std::string& anmPath,
			const std::string& clipName = "");

		/// <summary>
		/// リソースを取得する
		/// </summary>
		FbxResource* GetResource(const std::string& binPath) const;

		/// <summary>
		/// リソースを解放する
		/// </summary>
		void Unload(const std::string& binPath);

		/// <summary>
		/// 全キャッシュをクリアする
		/// </summary>
		void Clear();

	private:
		/// <summary>
		/// 保留中のロードデータ
		/// </summary>
		struct PendingLoad
		{
			std::string BinPath;
			FbxResource::LoadedBinData Data;
		};

		/// <summary>
		/// リソースキャッシュ
		/// </summary>
		std::unordered_map<std::string, std::unique_ptr<FbxResource>> mCache;

		/// <summary>
		/// 排他制御用ミューテックス
		/// </summary>
		mutable std::mutex mMutex;

		/// <summary>
		/// 並列パース用のスレッドプール
		/// </summary>
		utility::ThreadPool mLoadThreadPool;

		/// <summary>
		/// スレッドプールが起動済みかのフラグ
		/// </summary>
		bool mLoadThreadPoolStarted = false;

		/// <summary>
		/// GPU リソース化待ちのパース結果リスト
		/// </summary>
		std::vector<PendingLoad> mPendingParsed;
	};
}
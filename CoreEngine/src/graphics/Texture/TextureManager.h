#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>
#include<Utility/Thread/ThreadPool.h>

#include<filesystem>
#include<unordered_map>
#include<vector>
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
		/// �e�N�X�`���̎擾�A�~���[�h�Ȃ烍�[�h����B
		/// </summary>
		/// <param name="FilePath">�t�@�C���p�X</param>
		/// <param name="isSRGB">true�Ȃ�SRV��sRGB�Ƃ��ĉ��߂���(Albedo/Emissive�Ȃǐ F�e�N�X�`���p)</param>
		/// <returns>�Q�Ɨp�̃|�C���^</returns>
		Texture* GetOrLoad(const std::filesystem::path& FilePath, bool isSRGB = false);

		// Preload many textures at once. The CPU-only decode step (file read +
		// DirectXTex decode + mip generation) runs in parallel across a worker
		// thread pool; GPU resource creation/upload stays serialized on the
		// calling thread (same thread-safety story as GetOrLoad). Paths already
		// cached (matching isSRGB) are skipped. Intended for scene/loading-screen
		// bulk preloads where many textures are known up front.
		void PreloadBatch(const std::vector<std::filesystem::path>& FilePaths, bool isSRGB = false);

		/// <summary>
		/// ���ׂẴe�N�X�`�����������B
		/// </summary>
		void Clear();
	private:
		// Cache key: absolute path, plus an "|srgb" suffix so the same file loaded
		// with different isSRGB interpretations is never shared between entries
		// (shared by GetOrLoad and PreloadBatch, was previously duplicated inline).
		static std::string MakeCacheKey(const std::filesystem::path& FilePath, bool isSRGB);

		/// <summary>
		/// ���\�[�X�S��
		/// </summary>
		std::unordered_map<std::string, std::unique_ptr<Texture>> mResources;

		/// <summary>
		/// �r������
		/// </summary>
		std::mutex mMutex;

		// Worker pool used only by PreloadBatch() for parallel CPU-side decode.
		// Lazily started on first use (see PreloadBatch); GetOrLoad() never touches it.
		utility::ThreadPool mLoadThreadPool;
		bool mLoadThreadPoolStarted = false;
	};

}


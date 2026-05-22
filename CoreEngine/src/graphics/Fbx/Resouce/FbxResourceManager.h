#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include <unordered_map>
#include <memory>
#include <string>

#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{
	class FbxResource;

	class FbxResourceManager : public utility::Singleton<FbxResourceManager>
	{
		SINGLETON_CLASS(FbxResourceManager);
	public:
		SINGLETON_ACCESSOR(FbxResourceManager);

        /// <summary>
        /// binPath のリソースを返す。未ロードなら Load を実行してキャッシュする。
        /// anmPath を渡すと .anm も一緒にロードされる（初回のみ有効）。
        /// 失敗時は nullptr を返す。
        /// </summary>
        std::shared_ptr<FbxResource> Load(
            ID3D12GraphicsCommandList* cmdList,
            const std::string& binPath,
            const std::string& anmPath = "");

        /// <summary>キャッシュ済みリソースを取得する（未ロード時は nullptr）</summary>
        std::shared_ptr<FbxResource> Get(const std::string& binPath) const;

        /// <summary>参照が自分だけになったリソースをキャッシュから解放する</summary>
        void Unload(const std::string& binPath);

        /// <summary>全キャッシュをクリアする</summary>
        void Clear();

    private:
        std::unordered_map<std::string, std::shared_ptr<FbxResource>> mCache;
	};
}



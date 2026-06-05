#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<string>
#include<vector>
#include<unordered_map>
#include<memory>
#include<Utility/Export/Export.h>

namespace graphics
{
	class ModelResource;

	/// <summary>
	/// モデルリソースの管理
	/// </summary>
    class ENGINE_API ModelResourceManager : public utility::Singleton<ModelResourceManager>
    {
        SINGLETON_CLASS(ModelResourceManager);
    public:
        SINGLETON_ACCESSOR(ModelResourceManager);

        /// <summary>
        /// .bin をロードしてキャッシュに登録する。
        /// 既にキャッシュ済みならそれを返す (二重ロードしない)。
        /// </summary>
        std::shared_ptr<ModelResource> Load(const std::string& binPath);

        /// <summary>
        /// キャッシュ済みリソースに .anm を追加ロードする。
        /// 先に Load() を呼んでおく必要がある。
        /// 同じリソースに何度でも呼べる (クリップが末尾に追加される)。
        /// </summary>
        /// <summary>
        /// overrideName を指定するとクリップ名をその名前に上書きできる。
        /// 省略すると .anm ファイルに保存されている名前をそのまま使う。
        /// </summary>
        bool AppendAnimation(const std::string& binPath,
            const std::string& anmPath,
            const std::string& overrideName = "");

        /// <summary>
        /// キャッシュ済みリソースを取得 (未ロードなら nullptr)
        /// </summary>
        std::shared_ptr<ModelResource> GetResource(const std::string& binPath) const;

        /// <summary>参照がなくなったリソースをキャッシュから削除</summary>
        void Unload(const std::string& binPath);

        /// <summary>全キャッシュをクリア</summary>
        void Clear();

    private:
        std::unordered_map<std::string, std::shared_ptr<ModelResource>> mCache;
    };
}



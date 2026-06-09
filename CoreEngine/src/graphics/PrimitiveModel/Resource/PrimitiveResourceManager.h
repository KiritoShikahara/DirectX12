#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<string>
#include<unordered_map>
#include<memory>

#include<graphics/Fbx/Resource/FbxResource.h>


namespace graphics
{

	enum class PrimitiveType
	{
		Box,
		Sphere,
		Plane,
		Cylinder,
		Capsule,
	};

	class PrimitiveResourceManager : public utility::Singleton<PrimitiveResourceManager>
	{
		SINGLETON_CLASS(PrimitiveResourceManager);
	public:
		SINGLETON_ACCESSOR(PrimitiveResourceManager);

		/// <summary>
		/// デフォルトパラメーターでPrimitiveResourceを生成してキャッシュする。
		/// </summary>
		/// <returns></returns>
		void Initialize();

		/// <summary>キャッシュを全破棄する</summary>
		void Finalize();

		/// <summary>
		/// 名前からキャッシュされた PrimitiveResourceを返す。
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		FbxResource* GetResource(const std::string& name) const;

		/// <summary>
		/// カスタムパラメータでプリミティブを生成してキャッシュに登録する
		/// すでに同名のエントリがあれば上書きする
		/// </summary>
		FbxResource* CreateAndRegister(
			PrimitiveType type,
			const std::string& name,
			float p0 = 1.f, float p1 = 1.f, float p2 = 1.f,
			uint32_t div0 = 16, uint32_t div1 = 8);


	private:

		std::unique_ptr<FbxResource> CreatePrimitive(
			PrimitiveType type,
			float p0, float p1, float p2,
			uint32_t div0, uint32_t div1) const;

	private:
		std::unordered_map<std::string, std::unique_ptr<FbxResource>> mCache;


	};
}



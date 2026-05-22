#pragma once

#include<Utility/Singleton/Singleton.hpp>

namespace graphics
{
	/// <summary>
	/// FBXモデルの描画を担当するクラス。
	/// </summary>
	class FbxRenderer : public utility::Singleton<FbxRenderer>
	{
		SINGLETON_CLASS(FbxRenderer);
	public:
		SINGLETON_ACCESSOR(FbxRenderer);


	};
}



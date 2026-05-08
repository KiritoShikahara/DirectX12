#pragma once

#include<unordered_map>
#include<string_view>
#include<string>
#include<Utility/Export/Export.h>
#include<Utility/Singleton/Singleton.hpp>
#include<graphics/Dx12/Dx12Type.h>

namespace graphics
{
	class ENGINE_API ShaderManager : public utility::Singleton<ShaderManager>
	{
		SINGLETON_CLASS(ShaderManager);
	public:
		SINGLETON_ACCESSOR(ShaderManager);

		/// <summary>
		/// シェーダーの取得
		/// </summary>
		/// <param name="FileName">ファイルパス</param>
		/// <param name="EntryPoint">エントリー名</param>
		/// <param name="Target">バージョン</param>
		/// <returns></returns>
		Blob GetShader(std::string_view FileName, std::string_view EntryPoint, std::string_view Target);

	private:
		/// <summary>
		/// キーを作成する。
		/// ファイル名＋エントリポイント＋ターゲットでVSとかPSを区別する
		/// </summary>
		/// <param name="FileName">ファイルパス</param>
		/// <param name="EntryPoint">エントリー名</param>
		/// <param name="Target">バージョン</param>
		/// <returns></returns>
		std::string CreateKey(std::string_view FileName, std::string_view EntryPoint, std::string_view Target);

		/// <summary>
		/// シェーダーのコンパイル
		/// </summary>
		/// <param name="FileName">ファイルパス</param>
		/// <param name="EntryPoint">エントリー名</param>
		/// <param name="Target">バージョン</param>
		/// <returns></returns>
		Blob CompileShader(std::string_view FileName, std::string_view EntryPoint, std::string_view Target);

		/// <summary>
		/// シェーダーのコレクション
		/// </summary>
		std::unordered_map<std::string, Blob> mShaders;

	};
}



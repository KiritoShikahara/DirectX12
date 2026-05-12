#include"pch.h"
#include "ShaderManager.h"

namespace graphics
{
    /// <summary>
	/// シェーダーの取得
	/// </summary>
	/// <param name="FileName">ファイルパス</param>
	/// <param name="EntryPoint">エントリー名</param>
	/// <param name="Target">バージョン</param>
	/// <returns></returns>
	Blob ShaderManager::GetShader(std::string_view FileName, std::string_view EntryPoint, std::string_view Target)
	{
		//	キー作成
		std::string key = CreateKey(FileName, EntryPoint, Target);

		// キャッシュ確認
		auto it = mShaders.find(key);
		if (it != mShaders.end())
		{
			return it->second;
		}

		//	無ければコンパイル
		Blob shaderBlob = CompileShader(FileName, EntryPoint, Target);

		mShaders.emplace(key, shaderBlob);
		return shaderBlob;
	}

	/// <summary>
	/// キーを作成する。
	/// ファイル名＋エントリポイント＋ターゲットでVSとかPSを区別する
	/// </summary>
	/// <param name="FileName">ファイルパス</param>
	/// <param name="EntryPoint">エントリー名</param>
	/// <param name="Target">バージョン</param>
	/// <returns></returns>
	std::string ShaderManager::CreateKey(std::string_view FileName, std::string_view EntryPoint, std::string_view Target)
	{
		return std::string(FileName) + "|" + std::string(EntryPoint) + "|" + std::string(Target);
	}

	/// <summary>
	/// シェーダーのコンパイル
	/// </summary>
	/// <param name="FileName">ファイルパス</param>
	/// <param name="EntryPoint">エントリー名</param>
	/// <param name="Target">バージョン</param>
	/// <returns></returns>
	Blob ShaderManager::CompileShader(std::string_view FileName, std::string_view EntryPoint, std::string_view Target)
	{
		// UTF-8 -> UTF-16 変換
		std::string fileNameStr(FileName);
		int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, fileNameStr.c_str(), -1, nullptr, 0);
		if (sizeNeeded <= 0) return nullptr;

		std::wstring wideFileName(sizeNeeded, 0);
		MultiByteToWideChar(CP_UTF8, 0, fileNameStr.c_str(), -1, wideFileName.data(), sizeNeeded);
		// MultiByteToWideCharは末尾の\0を含めたサイズを返すため、wstringのサイズを調整
		if (!wideFileName.empty() && wideFileName.back() == L'\0') {
			wideFileName.pop_back();
		}

		std::string entryStr(EntryPoint);
		std::string targetStr(Target);

		UINT flags = 0;
#ifdef _DEBUG
		flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

		Blob shader = nullptr;
		Blob error = nullptr;

		HRESULT hr = D3DCompileFromFile(
			wideFileName.c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, // 将来的にカスタムHandlerへ拡張
			entryStr.c_str(),
			targetStr.c_str(),
			flags,
			0,
			shader.GetAddressOf(),
			error.GetAddressOf()
		);

		if (FAILED(hr))
		{
			if (error) {
				std::string errorMessage(
					static_cast<const char*>(error->GetBufferPointer()),
					error->GetBufferSize()
				);
				DEBUG_LOG(sys::eLogLevel::Error, std::format("Shader Compile Error: \n{}", errorMessage));
			}
			else {
				DEBUG_LOG(sys::eLogLevel::Error, std::format("D3DCompileFromFile failed. HR: 0x{:08X} Path: {}", (uint32_t)hr, fileNameStr));
			}
			return nullptr;
		}

		return shader;
	}


}

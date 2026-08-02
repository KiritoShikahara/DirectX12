#include "pch.h"
#include "ShaderManager.h"

// DXC headers
#include <dxcapi.h>
#include <filesystem>
#pragma comment(lib, "dxcompiler.lib")

namespace graphics
{
    /// <summary>
    /// シェーダーの取得
    /// </summary>
    /// <param name="FileName">ファイルパス</param>
    /// <param name="EntryPoint">エントリーポイント</param>
    /// <param name="Target">バージョン</param>
    /// <returns></returns>
    Blob ShaderManager::GetShader(std::string_view FileName, std::string_view EntryPoint, std::string_view Target)
    {
        // キー作成
        std::string key = CreateKey(FileName, EntryPoint, Target);

        // キャッシュ確認
        auto it = mShaders.find(key);
        if (it != mShaders.end())
        {
            return it->second;
        }

        // なければコンパイル
        Blob shaderBlob = CompileShader(FileName, EntryPoint, Target);

        mShaders.emplace(key, shaderBlob);
        return shaderBlob;
    }

    /// <summary>
    /// キーを作成する。
    /// ファイル名＋エントリーポイント＋ターゲット (VS と PS を区別する)
    /// </summary>
    std::string ShaderManager::CreateKey(std::string_view FileName, std::string_view EntryPoint, std::string_view Target)
    {
        return std::string(FileName) + "|" + std::string(EntryPoint) + "|" + std::string(Target);
    }

    /// <summary>
    /// DXC を使ってシェーダーをコンパイルする。
    /// 呼び出し側は Target に Shader Model 6.x 以降のプロファイル (vs_6_0 等) を渡すこと。
    /// 結果は ID3DBlob 互換の Blob として返す。
    /// </summary>
    Blob ShaderManager::CompileShader(std::string_view FileName, std::string_view EntryPoint, std::string_view Target)
    {
        // DZCインスタンス生成
        Microsoft::WRL::ComPtr<IDxcUtils>    utils;
        Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;

        HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, std::format("DxcCreateInstance (Utils) failed. HR: 0x{:08X}", (uint32_t)hr));
            return nullptr;
        }

        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, std::format("DxcCreateInstance (Compiler) failed. HR: 0x{:08X}", (uint32_t)hr));
            return nullptr;
        }

        // includeハンドラ
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
        hr = utils->CreateDefaultIncludeHandler(&includeHandler);
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, std::format("CreateDefaultIncludeHandler failed. HR: 0x{:08X}", (uint32_t)hr));
            return nullptr;
        }

		// UTF-8 文字列を UTF-16 に変換する。DXC は UTF-16 を使うため。
        std::string fileNameStr(FileName);
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, fileNameStr.c_str(), -1, nullptr, 0);
        if (sizeNeeded <= 0)
        {
            DEBUG_LOG(sys::eLogLevel::Error, std::format("MultiByteToWideChar failed. Path: {}", fileNameStr));
            return nullptr;
        }

        std::wstring wideFileName(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, fileNameStr.c_str(), -1, wideFileName.data(), sizeNeeded);
        // MultiByteToWideChar は終端の \0 を含むサイズを返すため調整
        if (!wideFileName.empty() && wideFileName.back() == L'\0')
        {
            wideFileName.pop_back();
        }

        // EntryPoint / Target も wstring に変換
        std::string entryStr(EntryPoint);
        std::string targetStr(Target);

        std::wstring wideEntry(entryStr.begin(), entryStr.end());
        std::wstring wideTarget(targetStr.begin(), targetStr.end());

        // シェーダーファイルのディレクトリを取得する。
        // DXC のデフォルトインクルードハンドラはカレントディレクトリ基準で解決するため、
        // FXC の D3D_COMPILE_STANDARD_FILE_INCLUDE と同様に
        // シェーダーファイルの親ディレクトリを -I で明示的に追加する必要がある。
        std::filesystem::path shaderDir =
            std::filesystem::path(wideFileName).parent_path();
        std::wstring wideShaderDir = shaderDir.wstring();

        // ソースファイルの読み込み
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
        hr = utils->LoadFile(wideFileName.c_str(), nullptr, &sourceBlob);
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, std::format("IDxcUtils::LoadFile failed. HR: 0x{:08X} Path: {}", (uint32_t)hr, fileNameStr));
            return nullptr;
        }

        DxcBuffer sourceBuffer{};
        sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
        sourceBuffer.Size = sourceBlob->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        // コンパイル引数の組み立て
        std::vector<LPCWSTR> args;

        // エントリーポイント / ターゲットプロファイル
        args.push_back(L"-E"); args.push_back(wideEntry.c_str());
        args.push_back(L"-T"); args.push_back(wideTarget.c_str());

        // シェーダーファイルのディレクトリをインクルードパスに追加する。
        // これにより、相対パス指定の #include がシェーダーと同じフォルダから解決される。
        args.push_back(L"-I");
        args.push_back(wideShaderDir.c_str());

#ifdef _DEBUG
        args.push_back(L"-Zi");  // デバッグ情報を埋め込む
        args.push_back(L"-Od");  // 最適化無効
        args.push_back(L"-Qembed_debug"); // PDB をシェーダーバイナリに埋め込む
#else
        args.push_back(L"-O3");  // 最大最適化
#endif

        // コンパイル実行
        Microsoft::WRL::ComPtr<IDxcResult> result;
        hr = compiler->Compile(
            &sourceBuffer,
            args.data(),
            static_cast<UINT32>(args.size()),
            includeHandler.Get(),
            IID_PPV_ARGS(&result)
        );

        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, std::format("IDxcCompiler3::Compile failed. HR: 0x{:08X} Path: {}", (uint32_t)hr, fileNameStr));
            return nullptr;
        }

        // エラー・警告メッセージの確認
        Microsoft::WRL::ComPtr<IDxcBlobUtf8> errorBlob;
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorBlob), nullptr);
        if (errorBlob && errorBlob->GetStringLength() > 0)
        {
            DEBUG_LOG(sys::eLogLevel::Error,
                std::format("Shader Compile Error:\n{}", errorBlob->GetStringPointer()));
        }

        // コンパイル結果の確認
        HRESULT compileStatus = S_OK;
        result->GetStatus(&compileStatus);
        if (FAILED(compileStatus))
        {
            return nullptr;
        }

        // バイトコードを取得
        Microsoft::WRL::ComPtr<IDxcBlob> dxcShader;
        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxcShader), nullptr);
        if (FAILED(hr) || !dxcShader)
        {
            DEBUG_LOG(sys::eLogLevel::Error, std::format("GetOutput (DXC_OUT_OBJECT) failed. HR: 0x{:08X}", (uint32_t)hr));
            return nullptr;
        }

        // IDxcBlob -> ID3DBlob へコピーする
        Blob shader;
        hr = D3DCreateBlob(dxcShader->GetBufferSize(), &shader);
        if (FAILED(hr))
        {
            DEBUG_LOG(sys::eLogLevel::Error, std::format("D3DCreateBlob failed. HR: 0x{:08X}", (uint32_t)hr));
            return nullptr;
        }

        std::memcpy(
            shader->GetBufferPointer(),
            dxcShader->GetBufferPointer(),
            dxcShader->GetBufferSize()
        );

        return shader;
    }

} // namespace graphics
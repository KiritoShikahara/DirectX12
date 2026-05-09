#pragma once

#ifndef PCH_H
#define PCH_H

//	Windows.hの無駄削除
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif


#define NOMINMAX
#include <windows.h>

#include<utility>
#include<type_traits>
#include<typeindex>
#include<typeinfo>

#include<vector>
#include<array>
#include<queue>
#include<map>
#include<unordered_map>
#include<set>
#include<unordered_set>

#include<string>
#include<string_view>
#include<filesystem>
#include<format>

#include<memory>
#include<optional>
#include<variant>
#include<concepts>
#include<coroutine>
#include <chrono>

#include<algorithm>
#include<functional>
#include<numeric>
#include<tuple>
#include<span>
#include<ranges>
#include<initializer_list>

#include<thread>
#include<mutex>
#include<atomic>

#include<iostream>
#include<sstream>
#include<fstream>

#include <dxgidebug.h>
#include<cassert>
#include<cstdint>

#include<d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <comdef.h>

#include<d3dx12.h>
#include<DirectXTex/DirectXTex.h>
#include<ImGui/imgui.h>

#include<system/Logger/Logger.h>
#include<system/ImGui/ImGuiManager.h>
#include<system/AssetPath/AssetPathManager.h>

//// デバック用機能
#if defined(_DEBUG)
    // デバッグビルドならデフォルトで有効
#define DEV_TOOL_ENABLED  (1)
#define ENABLE_ASSERT     (1)
#define DEBUG_DRAW_COLLISION (DEV_TOOL_ENABLED)
#else
    // リリースビルドでも開発ツールを使いたい場合はここを (1) にする
#define DEV_TOOL_ENABLED  (0)
#define ENABLE_ASSERT     (0)
#define DEBUG_DRAW_COLLISION (0)
#endif

#endif //PCH_H
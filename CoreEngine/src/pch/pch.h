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

#include<ImGui/imgui.h>
#include<system/Logger/Logger.h>

#endif //PCH_H
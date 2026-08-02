#pragma once

//	ホットリロード実装する可能性あるので前提で組みます。
#ifdef MYENGINE_AS_DLL
//	Dllの時
#ifdef MYENGINE_EXPORTS
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif
#else
//	libの時は何もしない
#define ENGINE_API	
#endif

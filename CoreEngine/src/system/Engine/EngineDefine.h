#pragma once

// デバック用マクロ
#ifdef _DEBUG

#define DEBUG_FBX 1
#define DEBUG_CAMERA 1
#define DEBUG_LIGHT 1
#define DEBUG_SPRITE 1
#define DEBUG_SOUND 1
#define DEBUG_TEXT 1
#define DEBUG_EFFECT 1

#else

#define DEBUG_FBX 0
#define DEBUG_SPRITE 0 
#define DEBUG_LIGHT 0
#define DEBUG_CAMERA 0
#define DEBUG_SOUND 0
#define DEBUG_TEXT 0
#define DEBUG_EFFECT 0

#endif // _DEBUG
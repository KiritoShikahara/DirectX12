#pragma once

#include <Utility/config/DebugConfig.h>

#ifndef DEFINES
#define DEFINES


// Scene
#define TEST_SCENE_NAME  "Test"
#define TITLE_SCENE_NAME "Title"
#define HUB_SCENE_NAME "Hub"
#define STATUS_UPGRADE_SCENE_NAME "StatusUpgrade"
#define MENU_SCENE_NAME "Menu"
#define GAME_SCENE_NAME "Game"

// Release(DEV_TOOL_ENABLED=0)は必ずTitleから開始する。
// Debug/Developは動作確認の効率を優先し、従来通りGameから開始する。
#if DEV_TOOL_ENABLED
#define START_SCENE_NAME GAME_SCENE_NAME
#else
#define START_SCENE_NAME TITLE_SCENE_NAME
#endif


#endif
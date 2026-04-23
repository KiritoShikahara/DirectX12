#pragma once
#include<filesystem>

namespace sys
{
	struct WindowContext
	{
		//	ウィンドウ名
		std::filesystem::path Title = "SK Engine";
		//	物理的なウィンドウサイズ（デスクトップ上）
		int Width = 1280;
		int Height = 720;
		// ゲームロジック・描画用の仮想サイズ（内部の仮想サイズ）
		int VirtualWidth = 1920;
		int VirtualHeight = 1080;
		//	フルスクにするかどうか true:フルスク
		bool IsFullScreen = false;
		//	カーソルを表示するかどうか true:表示する
		bool ShowCursor = true;
	};
}
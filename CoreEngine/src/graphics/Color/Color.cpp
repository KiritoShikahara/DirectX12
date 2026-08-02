#include<pch.h>
#include "Color.h"

namespace graphics
{
	//	赤
	const Color Color::Red = { 1.0f, 0.0f, 0.0f, 1.0f };

	//	青
	const Color Color::Blue = { 0.0f, 0.0f, 1.0f, 1.0f };

	//	緑
	const Color Color::Green = { 0.0f, 1.0f, 0.0f, 1.0f };

	//	黒
	const Color Color::Black = { 0.0f, 0.0f, 0.0f, 1.0f };

	//	白
	const Color Color::White = { 1.0f, 1.0f, 1.0f, 1.0f };

	//	シアン
	const Color Color::Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };

	//	マゼンタ
	const Color Color::Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

	//	黄
	const Color Color::Yellow = { 1.0f, 0.92f, 0.016f, 1.0f };

	//	灰
	const Color Color::Gray = { 0.5f, 0.5f, 0.5f, 1.0f };

	//	透明
	const Color Color::Clear = { 0.0f,0.0f,0.0f,0.0f };

	/// <summary>
	/// RGBを255で設定できるメソッド
	/// </summary>
	/// <param name="r">赤</param>
	/// <param name="g">緑</param>
	/// <param name="b">青</param>
	/// <param name="a">透明度</param>
	/// <returns>色</returns>
	Color Color::FromRGB(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
	}

}
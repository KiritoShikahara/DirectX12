#pragma once
#include<Utility/Export/Export.h>

namespace graphics
{
	struct Color
	{
		Color() noexcept : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
		Color(float r, float g, float b) noexcept : r(r), g(g), b(b), a(1.0f) {}
		Color(float r, float g, float b, float a) noexcept : r(r), g(g), b(b), a(a) {}
		Color(const Color& color) noexcept : r(color.r), g(color.g), b(color.b), a(color.a) {}

		//	赤
		static const Color Red;
		//	緑
		static const Color Green;
		//	青
		static const Color Blue;
		//	黒
		static const Color Black;
		//	白
		static const Color White;
		//	シアン
		static const Color Cyan;
		//	マゼンタ
		static const Color Magenta;
		//	黄
		static const Color Yellow;
		//	灰
		static const Color Gray;
		//	透明
		static const Color Clear;

		//	GPU や SIMD用データなどで使用するのでunion
		union
		{
			struct
			{
				float r, g, b, a;
			};
			float value[4];
		};

		/// <summary>
		/// RGBを255で設定できるメソッド
		/// </summary>
		/// <param name="r">赤</param>
		/// <param name="g">緑</param>
		/// <param name="b">青</param>
		/// <param name="a">透明度</param>
		/// <returns>色</returns>
		static Color FromRGB(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);

		bool operator==(const Color& other) const { return r == other.r && g == other.g && b == other.b && a == other.a; }
		bool operator!=(const Color& other) const { return !(*this == other); }

		Color operator*(float s) const { return Color(r * s, g * s, b * s, a * s); }
		Color operator+(const Color& c) const { return Color(r + c.r, g + c.g, b + c.b, a + c.a); }

		inline float& operator[](int index) { return value[index]; }
		inline float operator[](int index) const { return value[index]; }

		// DX12の ClearRenderTargetView にそのまま渡すためのヘルパー
		const float* GetRawPointer() const { return value; }

	};

}



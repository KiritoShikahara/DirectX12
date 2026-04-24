#pragma once
#include<Utility/Export/Export.h>

namespace graphics
{
	/// <summary>
	/// ディスクリプタヒープのスロット位置とサイズを保持する情報構造体
	/// </summary>
	struct ENGINE_API GDescriptorHeapInfo
	{
		/// <summary>開始スロットインデックス。-1 は未確保。</summary>
		int Index = -1;
		/// <summary>確保したスロット数</summary>
		int Size = 0;

		bool IsValid() const noexcept { return Index >= 0 && Size > 0; }
	};

}
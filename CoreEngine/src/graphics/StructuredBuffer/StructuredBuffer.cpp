#include"pch.h"
#include "StructuredBuffer.h"

#include<graphics/Dx12/Dx12Device.h>
#include<graphics/Dx12/RenderContext.h>

namespace graphics
{

	/// <summary>
	/// 現在のインデックスを取得するヘルパー
	/// dx12から取得する
	/// </summary>
	/// <returns></returns>
	uint32_t StructuredBuffer::GetCurrentIndex() const
	{
		return graphics::RenderContext::Get().GetFrameIndex();
	}
}
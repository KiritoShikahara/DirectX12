#pragma once
#include<Utility/Singleton/Singleton.hpp>

namespace graphics
{

	class RenderContext :public utility::Singleton<RenderContext>
	{
		SINGLETON_CLASS(RenderContext);
	public:
		SINGLETON_ACCESSOR(RenderContext);

		void SetFrameIndex(unsigned int frameIndex)
		{
			CurrentFrameIndex = frameIndex;
		}
		
		unsigned int GetFrameIndex() const
		{
			return CurrentFrameIndex;
		}

	private:
		unsigned int CurrentFrameIndex = 0;
	};
}
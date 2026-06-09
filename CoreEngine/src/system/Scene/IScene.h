#pragma once

namespace sys
{
	/// <summary>
	/// シーンの基底クラス
	/// </summary>
	class IScene
	{
	public:
		virtual bool Initialize() = 0;
		virtual void Finalize() {};
	};
}
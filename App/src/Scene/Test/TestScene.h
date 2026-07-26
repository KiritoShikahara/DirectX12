#pragma once

#include<system/Scene/IScene.h>

namespace scene
{
	class TestScene : public ::sys::IScene
	{
	public:
		virtual void Initialize()override;
		virtual void Finalize()override;

	private:
		// 繝ｪ繧ｽ繝ｼ繧ｹ隱ｭ縺ｿ霎ｼ縺ｿ
		void LoadResource();

		// 逕ｻ蜒・
		void CreateSprite();

		// Fbx
		void CreateFbx();

		// BGM縺ｨSE
		void CreateSound();

		// 繝｡繧､繝ｳ繧ｫ繝｡繝ｩ
		void CreateCamera();

		// 繝・ぅ繝ｬ繧ｯ繧ｷ繝ｧ繝ｳ繝ｩ繧､繝・
		void CreateLight();

		// 繝輔ぅ繝ｼ繝ｫ繝・
		void CreateField();

		// 譁・ｭ・
		void CreateText();

		// 繧ｨ繝輔ぉ繧ｯ繝・
		void CreateEffect();

		// 繧ｹ繧ｫ繧､繝懊ャ繧ｯ繧ｹ
		void CreateSkybox();

		// Shape
		void CreateShape();

	};
}





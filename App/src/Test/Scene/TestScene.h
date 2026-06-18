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
		// リソース読み込み
		void LoadResource();

		// 画像
		void CreateSprite();

		// Fbx
		void CreateFbx();

		// BGMとSE
		void CreateSound();

		// メインカメラ
		void CreateCamera();

		// ディレクションライト
		void CreateLight();

		// フィールド
		void CreateField();

		// 文字
		void CreateText();

		// エフェクト
		void CreateEffect();

		// スカイボックス
		void CreateSkybox();

	};
}





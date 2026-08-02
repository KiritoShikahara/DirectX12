#pragma once

#include <system/Scene/IScene.h>

namespace scene
{
	class TestScene : public ::sys::IScene
	{
	public:
		virtual void Initialize() override;
		virtual void Finalize() override;

	private:
		// リソース読み込み
		void LoadResource();

		// スプライト
		void CreateSprite();

		// Fbx
		void CreateFbx();

		// BGMとSE
		void CreateSound();

		// メインカメラ
		void CreateCamera();

		// ディレクショナルライト
		void CreateLight();

		// フィールド
		void CreateField();

		// テキスト
		void CreateText();

		// エフェクト
		void CreateEffect();

		// スカイボックス
		void CreateSkybox();

		// シェイプ
		void CreateShape();
	};
}
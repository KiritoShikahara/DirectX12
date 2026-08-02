#pragma once

#include<entt/entt.hpp>
#include<atomic>
#include<functional>
#include<memory>

namespace ecs
{
	/// <summary>
	/// 先読み進捗のカウンタ。CoreEngine側(FbxResourceManager/TextureManagerの
	/// PreloadBatch)はecs/Appのことを一切知らないため、進捗報告はこの構造体への
	/// 参照ではなく汎用のstd::function&lt;void(int)&gt;/std::function&lt;void()&gt;
	/// コールバック経由で行う(呼び出し側=App層がこの構造体への加算にラップする)。
	///
	/// Totalは「これから読み込む項目数が判明した時点」で加算されるため、
	/// 先読みの途中(FBX解析後にテクスチャ枚数が判明するタイミングなど)で
	/// 増えることがある(=パーセント表示が一時的に下がって見えることがある)。
	/// これは複数段階の先読みでは一般的な挙動であり、Totalが0の間は0%として扱う。
	/// </summary>
	struct LoadingProgress
	{
		std::atomic<int> Loaded{ 0 };
		std::atomic<int> Total{ 0 };
	};

	/// <summary>
	/// LoadingSceneが生成するコントローラーエンティティに付与するコンポーネント。
	///
	/// GPUリソース生成(VertexBuffer/IndexBuffer構築・テクスチャアップロード)を
	/// メインスレッドの描画と並行して背景スレッドから行うと、DirectX12のGPU同期を壊し
	/// ロードが完了しない/アプリが終了できなくなる不具合の原因になることが判明したため、
	/// CPU処理とGPU処理を明確にスレッド分離する設計にしている:
	///
	///   1. バックグラウンドスレッド(LoadingScene::Initialize内で起動)がCPU専用の
	///      先読み処理(ファイル解析・画像デコード。D3D12を一切呼ばない)を終えると
	///      IsCpuDoneをtrueにする。
	///   2. LoadingScreenUpdateSystemが毎フレームこれをポーリングし、trueになった瞬間
	///      メインスレッド上でResolveFn(GPUリソース生成)を同期的に呼び出す。
	///   3. ResolveFn完了直後、同じくメインスレッド上でOnCompleteを1回だけ呼び出す
	///      (実際のシーン切り替え等はそこで行う)。
	///
	/// IsCpuDoneをshared_ptr&lt;atomic&gt;にしているのは、バックグラウンドスレッドの
	/// ラムダにコピーで持たせるため(スレッドはLoadingSceneが破棄された後も安全に
	/// 書き込める必要がある。スレッド自体はLoadingScene::Finalizeでjoinするため、
	/// 実際にはスレッド生存期間はLoadingScene生存期間に収まるが、念のためコンポーネント
	/// 経由の間接参照ではなくshared_ptrで所有権を独立させておく)。
	/// </summary>
	struct LoadingScreenComponent
	{
		std::shared_ptr<std::atomic<bool>> IsCpuDone = std::make_shared<std::atomic<bool>>(false);

		/// <summary>バックグラウンドスレッドとLoadingScreenUpdateSystemが共有する進捗カウンタ</summary>
		std::shared_ptr<LoadingProgress> Progress = std::make_shared<LoadingProgress>();

		/// <summary>GPUリソース生成フェーズ。メインスレッド専用、IsCpuDone検知後に1回だけ呼ばれる</summary>
		std::function<void(LoadingProgress&)> ResolveFn;

		/// <summary>ResolveFn完了直後、メインスレッドから1回だけ呼ばれる</summary>
		std::function<void()> OnComplete;

		/// <summary>ResolveFn/OnCompleteを呼び出し済みかどうか(二重呼び出し防止)</summary>
		bool Resolved = false;
	};

	/// <summary>ローディング画面のスピナー(回転アイコン)Spriteに付与するタグ。
	/// LoadingScreenUpdateSystemが毎フレームTransform::Set2DRotationを進める</summary>
	struct LoadingSpinnerUiTag
	{
		float RotationRad = 0.0f;
	};

	/// <summary>進捗バーの塗り部分Spriteに付与するタグ。
	/// LoadingScreenUpdateSystemが毎フレームSprite::FillAmountへ進捗率(0-1)を書き込む</summary>
	struct LoadingProgressBarFillTag {};

	/// <summary>進捗パーセント表示のTextComponentに付与するタグ</summary>
	struct LoadingProgressTextTag {};
}

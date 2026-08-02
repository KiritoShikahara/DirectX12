#include "apppch.h"
#include "LoadingScreenUpdateSystem.h"
#include "LoadingScreenComponent.h"
#include<ecs/component/sprite/SpriteComponent.h>
#include<ecs/component/Text/TextComponent.h>

#include<algorithm>

namespace
{
	// スピナーの回転速度(ラジアン/秒)
	constexpr float kSpinnerRotationSpeed = 3.0f;
}

namespace ecs
{
	void LoadingScreenUpdateSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// スピナーの回転(演出用。TimeScaleの影響を受けないようrawDeltaTimeを使う。
		// ローディング中はゲームプレイのTimeScaleと無関係に一定速度で回したいため)
		registry.view<ecs::LoadingSpinnerUiTag, ecs::Transform>().each(
			[rawDeltaTime](ecs::LoadingSpinnerUiTag& spinner, ecs::Transform& transform)
			{
				spinner.RotationRad += kSpinnerRotationSpeed * rawDeltaTime;
				transform.Set2DRotation(spinner.RotationRad);
			});

		// バックグラウンドスレッドのCPU専用先読み(ファイル解析・画像デコード)完了を検知したら、
		// メインスレッド上で1回だけResolveFn(GPUリソース生成)→OnCompleteの順に呼ぶ。
		// GPUリソース生成を必ずメインスレッドのここで行うことで、背景スレッドからの
		// D3D12呼び出しによるGPU同期の破綻を避ける(LoadingScreenComponentのコメント参照)。
		// 同時にProgressを取り出しておき、バー/パーセント表示の更新にも使う
		// (コントローラーエンティティは1つだけ生成される想定)。
		std::shared_ptr<ecs::LoadingProgress> progress;
		registry.view<ecs::LoadingScreenComponent>().each(
			[&progress](ecs::LoadingScreenComponent& loading)
			{
				progress = loading.Progress;

				if (loading.Resolved) return;
				if (!loading.IsCpuDone->load(std::memory_order_acquire)) return;

				loading.Resolved = true;
				if (loading.ResolveFn) loading.ResolveFn(*loading.Progress);
				if (loading.OnComplete) loading.OnComplete();
			});

		if (!progress) return;

		// Totalが未判明(0)の間は0%扱い。読み込みが進むほどTotalが動的に増える
		// ことがあるため、進捗率は毎フレーム再計算する(詳細はLoadingProgressのコメント参照)。
		const int loaded = progress->Loaded.load(std::memory_order_relaxed);
		const int total = progress->Total.load(std::memory_order_relaxed);
		const float percent = (total > 0)
			? std::clamp(static_cast<float>(loaded) / static_cast<float>(total), 0.0f, 1.0f)
			: 0.0f;

		// LoadingProgressBarFillTag/LoadingProgressTextTagは無データのタグ型のため、
		// EnTTの.each()はコールバック引数からタグ自体を省略する(空コンポーネント最適化)。
		registry.view<ecs::LoadingProgressBarFillTag, ecs::Sprite>().each(
			[percent](ecs::Sprite& sprite)
			{
				sprite.FillAmount = percent;
			});

		registry.view<ecs::LoadingProgressTextTag, ecs::TextComponent>().each(
			[percent](ecs::TextComponent& text)
			{
				const int percentInt = static_cast<int>(percent * 100.0f + 0.5f);
				text.Text = std::to_wstring(percentInt) + L"%";
			});
	}
}

#include "apppch.h"
#include "LoadingScreenUpdateSystem.h"
#include "LoadingScreenComponent.h"
#include<ecs/component/sprite/SpriteComponent.h>
#include<ecs/component/Text/TextComponent.h>

#include<algorithm>

namespace
{
	constexpr float kSpinnerRotationSpeed = 3.0f;
}

namespace ecs
{
	void LoadingScreenUpdateSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		// スピナーの回転、演出用。TimeScaleの影響を受けないようrawDeltaTimeを使う
		registry.view<ecs::LoadingSpinnerUiTag, ecs::Transform>().each(
			[rawDeltaTime](ecs::LoadingSpinnerUiTag& spinner, ecs::Transform& transform)
			{
				spinner.RotationRad += kSpinnerRotationSpeed * rawDeltaTime;
				transform.Set2DRotation(spinner.RotationRad);
			});

		// バックグラウンドスレッドのCPU専用先読み完了を検知したら、メインスレッド上で1回だけResolveFn→OnCompleteの順に呼ぶ。同時にProgressを取り出しバー/パーセント表示の更新にも使う
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

		// Totalが未判明の間は0%扱い。読み込みが進むほどTotalが動的に増えることがあるため進捗率は毎フレーム再計算する
		const int loaded = progress->Loaded.load(std::memory_order_relaxed);
		const int total = progress->Total.load(std::memory_order_relaxed);
		const float percent = (total > 0)
			? std::clamp(static_cast<float>(loaded) / static_cast<float>(total), 0.0f, 1.0f)
			: 0.0f;

		// LoadingProgressBarFillTag/LoadingProgressTextTagは無データのタグ型のため、EnTTのeachはコールバック引数からタグ自体を省略する
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

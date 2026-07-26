#include "apppch.h"
#include "WeaponIconBarSystem.h"

#include"PlayerUiTag.h"

#include<system/Player/Weapon/Inventory/WeaponInventoryComponent.h>
#include<system/Player/Weapon/WeaponCooldownRegistry.h>
#include<system/Player/Weapon/WeaponIconRegistry.h>
#include<graphics/Text/Renderer/TextRenderer.h>
#include<Tag/EntityTag.h>

#include<array>
#include<algorithm>

namespace
{
	// GameSceneFactory::CreateWeaponIconBar(kColumns=5 * kRows=2)と一致させること
	constexpr int kSlotCount = 10;

	/// <summary>1スロット分の今フレームの表示状態(先に全スロット分まとめて計算しておく)</summary>
	struct SlotState
	{
		bool HasWeapon = false;
		const char* IconPath = nullptr;
		int Level = 0;
		bool ShowCooldown = false;   // クールダウン中(オーバーレイ・テキストとも表示)か
		float FillRatio = 0.0f;      // 残り/最大 [0,1]。オーバーレイのFillAmountにそのまま使う
		float RemainingSeconds = 0.0f;
	};
}

namespace ecs
{
	void WeaponIconBarSystem::Update(entt::registry& registry, float deltaTime, float rawDeltaTime)
	{
		auto playerView = registry.view<PlayerTag, WeaponInventoryComponent>();
		if (playerView.begin() == playerView.end()) return;

		const auto& inventory = registry.get<WeaponInventoryComponent>(*playerView.begin());

		// スロットごとの表示状態を先にまとめて計算する。
		// Icon/Overlay/Textが別エンティティ(別view)のため、同じ計算をそれぞれで
		// 繰り返さないようにするための下ごしらえ。
		std::array<SlotState, kSlotCount> states = {};
		const int weaponCount = std::min(static_cast<int>(inventory.Weapons.size()), kSlotCount);

		for (int i = 0; i < weaponCount; ++i)
		{
			const entt::entity weaponEntity = inventory.Weapons[i];
			if (!registry.valid(weaponEntity)) continue;

			const auto* weapon = registry.try_get<WeaponComponent>(weaponEntity);
			if (weapon == nullptr) continue;

			SlotState& state = states[i];
			state.HasWeapon = true;
			state.IconPath = weaponutil::GetWeaponIconPath(weapon->Type);
			state.Level = weapon->Level;

			float remaining = 0.0f, maxCooldown = 0.0f;
			if (weaponutil::TryGetWeaponCooldown(registry, weaponEntity, *weapon, remaining, maxCooldown)
				&& maxCooldown > 0.0f)
			{
				remaining = std::max(0.0f, remaining);
				if (remaining > 0.0f)
				{
					state.ShowCooldown = true;
					state.RemainingSeconds = remaining;
					state.FillRatio = std::clamp(remaining / maxCooldown, 0.0f, 1.0f);
				}
			}
		}

		// アイコン・オーバーレイ(いずれもSprite)の反映
		registry.view<WeaponIconSlotTag, Sprite>().each(
			[&](WeaponIconSlotTag& slot, Sprite& sprite)
			{
				if (slot.SlotIndex < 0 || slot.SlotIndex >= kSlotCount) return;
				const SlotState& state = states[slot.SlotIndex];

				if (slot.Element == eWeaponIconElement::Icon)
				{
					sprite.IsVisible = state.HasWeapon;
					if (state.HasWeapon)
					{
						sprite.Texture = ::graphics::TextureManager::Get().GetOrLoad(state.IconPath);
					}
				}
				else if (slot.Element == eWeaponIconElement::Overlay)
				{
					sprite.IsVisible = state.HasWeapon && state.ShowCooldown;
					sprite.FillAmount = state.FillRatio;
				}
			});

		// テキスト(残り秒数・武器レベル)の反映
		auto& textRenderer = ::graphics::TextRenderer::Get();
		registry.view<WeaponIconSlotTag, TextComponent>().each(
			[&](WeaponIconSlotTag& slot, TextComponent& text)
			{
				if (slot.SlotIndex < 0 || slot.SlotIndex >= kSlotCount) return;
				const SlotState& state = states[slot.SlotIndex];

				if (slot.Element == eWeaponIconElement::Text)
				{
					// 残りクールダウン秒数。クールダウン中のみ表示する
					const bool show = state.HasWeapon && state.ShowCooldown;
					text.IsVisible = show;
					if (!show) return;

					wchar_t buf[16];
					swprintf_s(buf, L"%.1fs", state.RemainingSeconds);
					text.Text = buf;
				}
				else if (slot.Element == eWeaponIconElement::LevelText)
				{
					// 武器レベル。クールダウンとは独立して、所持している間は常時表示する
					text.IsVisible = state.HasWeapon;
					if (!state.HasWeapon) return;

					wchar_t buf[16];
					swprintf_s(buf, L"Lv.%d", state.Level);
					text.Text = buf;
				}
				else
				{
					return;
				}

				// 水平中央揃え(PerkSelectSystemと同じ手法)。基準はスロット中心(slot.CenterX、不変)。
				const float textWidth = textRenderer.MeasureWidth(text.Text, text.Size);
				text.X = slot.CenterX - textWidth * 0.5f;
			});
	}
}

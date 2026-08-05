#pragma once

namespace ecs
{
	struct PlayerHpBarTag {};

	///<summary>
	///必殺技ゲージのスプライトを識別するタグ。PlayerUltimateGaugeSystemがFillAmountを撃破数の充填率で駆動する。HPバーと同じ画像を左右反転・青色で右下に配置したもの
	///</summary>
	struct PlayerUltimateGaugeTag {};

	///<summary>
	///経験値バーのスプライトを識別するタグ。PlayerExpBarSystemがFillAmountを現在経験値/次レベルに必要な経験値の比率で駆動する
	///</summary>
	struct PlayerExpBarTag {};

	///<summary>
	///現在レベル表示Lv.Xのテキストを識別するタグ。PlayerExpBarSystemが更新する
	///</summary>
	struct PlayerLevelTextTag {};

	///<summary>
	///所持武器アイコンバーの1スロットを構成する要素
	///</summary>
	enum class eWeaponIconElement
	{
		Icon,        // 武器アイコン本体
		Overlay,     // クールダウン進捗の黒半透明オーバーレイ、Radialで時計回りに消える
		Text,        // 残りクールダウン秒数、アイコン中央
		LevelText,   // 武器レベル、アイコン右下
		ControlIcon, // 操作方法アイコン(発動キー/ボタン)、アイコン左上。操作方法を持たない武器や対応アイコンが無いデバイスでは非表示
	};

	///<summary>
	///所持武器アイコンバーの1スロットを識別するタグ。SlotIndexはWeaponInventoryComponent::Weaponsのインデックスに対応し、Icon/Overlay/Text/LevelTextの4エンティティが同じSlotIndexを共有する
	///</summary>
	struct WeaponIconSlotTag
	{
		int SlotIndex = 0;
		eWeaponIconElement Element = eWeaponIconElement::Icon;

		///<summary>
		///Text/LevelText用、水平中央揃えの基準X座標。TextComponent::Xは幅に応じて毎フレーム書き換わるため、基準値は別途ここに持たないと位置がズレ続ける
		///</summary>
		float CenterX = 0.0f;
	};
}
